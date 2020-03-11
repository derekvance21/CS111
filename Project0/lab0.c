/*
NAME: Derek Vance
EMAIL: dvance@g.ucla.edu
ID: 604970765
*/

#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void causeSegFault() {
    char* ptr = NULL;
    *ptr = 'X';
}

void catchSignal(int sig) {
    if (sig == SIGSEGV)
        fprintf(stderr, "Segmentation fault caught\n");
    exit(4);
}

int main(int argc, char** argv) {

    static struct option long_options[] = {
        {"input", required_argument, NULL, 1 },
        {"output", required_argument, NULL, 2 },
        {"segfault", no_argument, NULL, 3 },
        {"catch", no_argument, NULL,  4 },
        {0, 0, 0, 0}
    };

    int c;
    char* inFile = NULL;
    char* outFile = NULL; 
    int segFault = 0;
    int catch = 0;

    opterr = 0;

    while ( (c = getopt_long(argc, argv, "", long_options, NULL)) != -1 ) {
        switch(c) {
            case 1:
                inFile = optarg;
                break;
            case 2:
                outFile = optarg;
                break;
            case 3:
                segFault = 1;
                break;
            case 4:
                catch = 1;
                break;
            default:
                fprintf(stderr, "Unrecognized option: %s\n", argv[optind - 1]);
                exit(1);
        }
    }
    
    if (optind < argc) {
        fprintf(stderr, "Unrecognized argument: %s\n", argv[optind]);
        exit(1);
    }

    int infd = 0;
    int outfd = 1;

    if (inFile) {
        infd = open(inFile, O_RDONLY);
        if (infd == -1) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(2);
        }
        close(0);
        if (dup(infd) == -1) {
            fprintf(stderr, "%s\n", strerror(errno));
        }
        close(infd);
    }

    if (outFile) {
        outfd = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (outfd == -1) {
            fprintf(stderr, "%s\n", strerror(errno));
            exit(3);
        }
        close(1);
        if (dup(outfd) == -1) {
            fprintf(stderr, "%s\n", strerror(errno));
        }
        close(outfd);
    }

    if (catch) {
        signal(SIGSEGV, catchSignal);
    }

    if (segFault) {
        causeSegFault();
    }

    char buffer[1];

    while(read(0, buffer, 1)) {
        if (write(1, buffer, 1) != 1) {
            fprintf(stderr, "%s\n", strerror(errno));
        }
    }

    exit(0);

}