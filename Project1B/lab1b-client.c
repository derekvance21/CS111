/*
NAME: Derek Vance
EMAIL: dvance@g.ucla.edu
ID: 604970765
*/

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <string.h>
#include <poll.h>
#include <termios.h>
#include <ulimit.h>
#include <zlib.h>
#include <assert.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384

char crlf[2] = "\r\n";

void error(char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

int def(unsigned char* msg, int msgsize, unsigned char* dest, int* destsize, int level)
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
            /*if (write(destfd, out, have) != have) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }*/
            *destsize += have;
            unsigned int i;
            for (i = 0; i < have; i++) {
                dest[i] = out[i];
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

int inf(unsigned char* msg, int msgsize, int destfd)
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
            unsigned int i;
            // do byte by byte output, with lf to crlf conversion
            for (i = 0; i < have; i++) {
                int failure = 0;
                if (out[i] == '\n') {
                    if (write(destfd, crlf, 2) != 2)
                        failure = 1;
                }
                else {
                    if (write(destfd, out + i, 1) != 1)
                        failure = 1;
                }
                if (failure) {
                    (void)inflateEnd(&strm);
                    return Z_ERRNO;
                }
            }
            /*if (write(destfd, out, have) != have) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }*/ 
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

struct termios orig_termios;


void resetTerminalMode() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios) < 0)
        fprintf(stderr, "error resetting terminal to orig_termios\n\r");
}

void enableTerminalMode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) < 0)
        fprintf(stderr, "error getting attr for orig_termios\n");
    atexit(resetTerminalMode);
    struct termios new_termios;
    if (tcgetattr(STDIN_FILENO, &new_termios) < 0){
        fprintf(stderr, "error getting attr for new_termios\n");
    }
    new_termios.c_iflag = ISTRIP;	/* only lower 7 bits	*/
    new_termios.c_oflag = 0;		/* no processing	*/
    new_termios.c_lflag = 0;		/* no processing	*/
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

int tBufferSize = 5;

int main(int argc, char **argv)
{
    // OPTION HANDLING
    static struct option long_options[] = {
        {"log", required_argument, NULL,  1},
        {"port", required_argument, NULL, 2},
        {"compress", no_argument, NULL, 3},
        {0, 0, 0, 0}
    };
    int c, logfd;
    int sockfd, portno, n, i;
    int log = 0;
    int port = 0;
    int compress = 0;
    while ( (c = getopt_long(argc, argv, "", long_options, NULL)) != -1 ) {
        switch(c) {
            case(1):
                log = 1;
                ulimit(UL_SETFSIZE, 10000);
                logfd = open(optarg, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                if (logfd < 0) error("ERROR opening log file");
                break;
            case(2):
                portno = atoi(optarg);
                port = 1;
                break;
            case(3):
                compress = 1;
                break;
            default:
                fprintf(stderr, "Unrecognized option: %s\n", argv[optind - 1]);
                exit(1);
        }
    }

    if (!port) error("ERROR port number option required");

    if (optind < argc) {
        fprintf(stderr, "Unrecognized argument: %s\n", argv[optind]);
        exit(1);
    }

    struct sockaddr_in serv_addr;
    struct hostent *server;

    unsigned char buffer[256];
    char tBuffer[tBufferSize];
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    struct pollfd pollfds[2] = {
        {STDIN_FILENO, POLLIN | POLLHUP | POLLERR, 0}, // stdin
        {sockfd, POLLIN | POLLHUP | POLLERR, 0} // socket
    };

    enableTerminalMode();
    
    while (1) {
        if ((poll(pollfds, 2, 0)) < 0)
            error("ERROR polling");
        if ((pollfds[0].revents&POLLIN) == POLLIN) {
            n = read(STDIN_FILENO, tBuffer, tBufferSize);
            if (n < 0) error("ERROR reading from terminal");
            for (i = 0; i < n; i++) {
                unsigned char ctosend;
                if (tBuffer[i] == '\r' || tBuffer[i] == '\n') {
                    write(1, crlf, 2);
                    ctosend = '\n';
                }
                else {
                    write(1, tBuffer + i, 1);
                    ctosend = tBuffer[i];
                }
                if (compress) {
                    int destsize = 0;
                    unsigned char dest[16];
                    if (def(&ctosend, 1, dest, &destsize, -1) != Z_OK)
                        error("ERROR sending lf compression to server");
                    else 
                        write(sockfd, dest, destsize);
                    if (log) {
                        dprintf(logfd, "SENT %d bytes: ", destsize);
                        write(logfd, dest, destsize);
                        dprintf(logfd, "\n");
                    }
                }
                else {
                    write(sockfd, &ctosend, 1);
                    if (log) {
                        dprintf(logfd, "SENT %d bytes: ", 1);
                        write(logfd, &ctosend, 1);
                        dprintf(logfd, "\n");
                    }
                }   
            }
        }
        if ((pollfds[1].revents&POLLIN) == POLLIN) {
            bzero(buffer,256);
            n = read(sockfd, buffer, 255);
            if (n < 0) error("ERROR reading from socket");
            if (n == 0) break;
            if (log) {
                dprintf(logfd, "RECEIVED %d bytes: ", n);
                write(logfd, buffer, n);
                dprintf(logfd, "\n");
            }
            if (compress) {
                inf(buffer, n, 1);
            }
            else {
                for (i = 0; i < n; i++) {
                    if (buffer[i] == '\n') {
                        write(1, crlf, 2);
                    }
                    else {
                        write(1, buffer + i, 1);
                    }
                }
            }
        }
        if ((pollfds[1].revents&POLLHUP) || (pollfds[1].revents&POLLERR))
            break;
    }
    return 0;
}
