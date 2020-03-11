/*
NAME: Derek Vance
EMAIL: dvance@g.ucla.edu
ID: 604970765
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <errno.h>
#include <signal.h> 
#include <getopt.h>
#include <assert.h>
#include <zlib.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384


void error(char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

int def(unsigned char* msg, int msgsize, int destfd, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        if (msgsize > CHUNK) {
            strm.avail_in = CHUNK;
            msgsize = msgsize - CHUNK;
        }
        else {
            strm.avail_in = msgsize;
            msgsize = 0;
        }
        flush = msgsize ? Z_NO_FLUSH : Z_FINISH;
        strm.next_in = msg;
        if (msgsize)
            msg = msg + CHUNK;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (write(destfd, out, have) != have) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
            
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

int inf(unsigned char* msg, int msgsize, unsigned char* dest, int* destsize)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        if (msgsize > CHUNK) {
            strm.avail_in = CHUNK;
            msgsize = msgsize - CHUNK;
        }
        else {
            strm.avail_in = msgsize;
            msgsize = 0;
        }
        if (strm.avail_in == 0)
            break;
        // strm.next_in = in;
        strm.next_in = msg;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            
            if (have > 16) error("ERROR not enough buffer size for terminal input");
            unsigned int i;
            *destsize += have;
            for (i = 0; i < have; i++) {
                if (out[i] == '\r') {
                    dest[i] = '\n';
                }
                else {
                    dest[i] = out[i];
                }
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}


int main(int argc, char **argv)
{
    int sockfd, newsockfd, portno, rc;
    socklen_t clilen;
    // OPTION HANDLING
    static struct option long_options[] = {
        {"port", required_argument, NULL, 1},
        {"compress", no_argument, NULL, 2},
        {0, 0, 0, 0}
    };

    int c;
    int port = 0;
    int compress = 0;
    while ( (c = getopt_long(argc, argv, "", long_options, NULL)) != -1 ) {
        switch(c) {
            case(1):
                portno = atoi(optarg);
                port = 1;
                break;
            case(2):
                compress = 1;
                break;
            default:
                fprintf(stderr, "Unrecognized option: %s\n", argv[optind - 1]);
                exit(1);
        }
    }

    if (!port) error("ERROR port number option required");
    
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if (optind < argc) {
        fprintf(stderr, "Unrecognized argument: %s\n", argv[optind]);
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
            sizeof(serv_addr)) < 0) 
            error("ERROR on binding");
    listen(sockfd,5);
    clilen = (socklen_t)sizeof(cli_addr);
    newsockfd = accept(sockfd, 
    (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) error("ERROR on accept");
    close(sockfd);

    int toShell[2];
    int fromShell[2];
    if (pipe(toShell) < 0 || pipe(fromShell) < 0) {
        error("ERROR on piping");
    }
    rc = fork();
    if (rc < 0) {
        error("ERROR on forking");
    }
    else if (rc == 0) { // child
        close(0);
        dup(toShell[0]);
        close(1);
        dup(fromShell[1]);
        close(2);
        dup(fromShell[1]);
        close(toShell[0]);
        close(toShell[1]);
        close(fromShell[0]);
        close(fromShell[1]);

        char* run[2] = {strdup("/bin/bash"), NULL};
        if (execvp(run[0], run) < 0) 
            error("ERROR on exec call");
    }
    else { // parent
        close(toShell[0]);
        close(fromShell[1]);
    }

    struct pollfd pollfds[2] = {
        {newsockfd, POLLIN | POLLHUP | POLLERR, 0},
        {fromShell[0], POLLIN | POLLHUP | POLLERR, 0}
    };
    int pollStatus, status;
    unsigned char buffer[256];
    while (1) {
        if ((pollStatus = poll(pollfds, 2, 0)) < 0)
            error("ERROR on poll");
        if ((pollfds[0].revents&POLLIN) == POLLIN) {
            bzero(buffer,256);
            n = read(newsockfd,buffer,255);
            if (n < 0) error("ERROR reading from socket");
            if (n == 0) continue;
            unsigned char inflatebuf[16];
            int destsize = 0;
            if (compress) {
                if (inf(buffer, n, inflatebuf, &destsize) != Z_OK)
                    error("ERROR inflating message from client");
            }
            int i;
            if (compress) n = destsize;
            for (i = 0; i < n; i++) {
                if ((compress && (inflatebuf[i] == '\003')) || (!compress && (buffer[i] == '\003'))) {
                    if (kill(rc, SIGINT) < 0) error("ERROR killing shell process");
                    break;
                }
                if ((compress && (inflatebuf[i] == '\004')) || (!compress && (buffer[i] == '\004'))) {
                    // write(toShell[1], buffer + i, 1);
                    close(toShell[1]);
                    break;
                }
                else {
                    if (compress) {
                        write(toShell[1], inflatebuf + i, 1);
                    }
                    else {
                        write(toShell[1], buffer + i, 1);
                    }
                }
            }
        }
        if (pollfds[1].revents&POLLIN) {
            bzero(buffer, 256);
            n = read(fromShell[0], buffer, 255);
            if (n < 0) error("ERROR reading from shell");
            if (compress) {
                if (def(buffer, n, newsockfd, -1) != Z_OK)
                    error("ERROR compressing server message to client");
            }
            else {
                n = write(newsockfd, buffer, n);
                if (n < 0) error("ERROR writing to socket");
            }
        }
        if ((pollfds[1].revents&POLLHUP) || (pollfds[1].revents&POLLERR)) {
            if (waitpid(rc, &status, 0) < 0) error("ERROR waiting for child process");
            break;
        }
        if ((pollfds[0].revents&POLLHUP) || (pollfds[0].revents&POLLERR)) {
            close(toShell[1]);
            break;
        }

    } /* end of while */
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n\r", status & 0x007f, WEXITSTATUS(status));
    close(newsockfd);
    return 0; 
}
