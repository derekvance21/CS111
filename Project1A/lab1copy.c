/*
NAME: Derek Vance
EMAIL: dvance@g.ucla.edu
ID: 604970765
*/

// c_iflag = ISTRIP;	/* only lower 7 bits	*/
// c_oflag = 0;		/* no processing	*/
// c_lflag = 0;		/* no processing	*/

#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <poll.h>

struct termios orig_termios;

int termBufSize = 10;
int shellBufSize = 256;

void resetTerminalMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableTerminalMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(resetTerminalMode);
    struct termios new_termios = orig_termios;
    new_termios.c_iflag = ISTRIP;	/* only lower 7 bits	*/
    new_termios.c_oflag = 0;		/* no processing	*/
    new_termios.c_lflag = 0;		/* no processing	*/
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

int main(int argc, char** argv) {
    static struct option long_options[] = {
        {"shell", no_argument, NULL,  1 },
        {0, 0, 0, 0}
    };
    int c;
    int shell = 0;
    while ( (c = getopt_long(argc, argv, "", long_options, NULL)) != -1 ) {
        switch(c) {
            case(1):
                shell = 1;
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

    enableTerminalMode();

    int rc;
    int fdTtoS[2]; // Terminal (1) to Shell (0)
    int fdStoT[2]; // Shell (1, 2) to Terminal (0)

    if (shell) {
        if (pipe(fdTtoS) < 0 || pipe(fdStoT) < 0) {
            fprintf(stderr, "pipe failed\r\n");
            exit(1);
        }

        rc = fork();
        if (rc < 0) {
            fprintf(stderr, "fork failed\r\n");
            exit(1);
        }
        else if (rc == 0) { // child : Shell
            close(0);
            dup(fdTtoS[0]);

            close(1);
            dup(fdStoT[1]);

            close(2);
            dup(fdStoT[1]);

            close(fdStoT[0]);
            close(fdStoT[1]);
            close(fdTtoS[0]);
            close(fdTtoS[1]);
            
            char* run[2] = {strdup("/bin/bash"), NULL};

            if (execvp(run[0], run) < 0) {
                fprintf(stderr, "exec call failed; %s\n\r", strerror(errno));
                exit(1);
            }
            
            
        }
        else { // parent : Terminal
            close(fdTtoS[0]);
            close(fdStoT[1]);
        }
    }

    int done = 0;
    char crlf[2];
    crlf[0] = '\r';
    crlf[1] = '\n';
    char ctrld[4] = "^D\n\r";
    char ctrlc[4] = "^C\n\r";
    int terminal = 1;

    struct pollfd pollfds[2] = {
        {0, POLLIN | POLLHUP | POLLERR, 0},
        {fdStoT[0], POLLIN | POLLHUP | POLLERR, 0}
    };

    while (1) {
        int pollStatus = poll(pollfds, 2, 0);
        if (pollStatus == -1) {
            fprintf(stderr, "%s\n\r", strerror(errno));
            exit(1);
        }
        if (terminal && (pollfds[0].revents&POLLIN) == POLLIN) { // terminal has input to be read from keyboard
            char tbuf[termBufSize];
            int termBytes = read(STDIN_FILENO, tbuf, termBufSize); // read keyboard input
            int i;
            for (i = 0; i < termBytes; i++) {
                if (tbuf[i] == '\004') {
                    write(1, ctrld, 4);
                    close(fdTtoS[1]);
                    terminal = 0;
                    break;
                }
                else if (tbuf[i] == '\r' || tbuf[i] == '\n') {
                    write(1, crlf, 2);
                    if (shell) {
                        write(fdTtoS[1], crlf + 1, 1);
                    }
                }
                else if (tbuf[i] == '\003') {
                    write(1, ctrlc, 4);
                    if (shell && kill(rc, SIGINT) == -1) {
                        fprintf(stderr, "%s\n\r", strerror(errno));
                    }
                }
                else {
                    write(1, tbuf + i, 1);
                    if (shell && (write(fdTtoS[1], tbuf + i, 1) == -1)) {
                        fprintf(stderr, "%s\n\r", strerror(errno));
                    }
                }
            }
        }
        if (!shell && !terminal) {
            break;
        }
        if (shell && (((pollfds[1].revents&POLLIN) == POLLIN) || ((pollfds[1].revents&POLLHUP) == POLLHUP))) { // terminal has input to be read from shell
            char sbuf[shellBufSize];
            int shellBytes = read(fdStoT[0], sbuf, shellBufSize); // read output from shell
            if (shellBytes == -1) {
                fprintf(stderr, "read from shell failed: %s\n\r", strerror(errno));
                exit(1);
            }
            int i;
            for (i = 0; i < shellBytes; i++) {
                if (sbuf[i] == '\n') {
                    write(1, crlf, 2);
                }
                else if (sbuf[i] == '\004') {
                    done = 1;
                    break;
                }
                else {
                    write(1, sbuf + i, 1);
                }
            }
            if (done)
                break;
            if ((pollfds[1].revents&POLLHUP) == POLLHUP) {
                int status;
                waitpid(rc, &status, 0); // THE MOST IMPORTANT PART
                char signal = status;
                fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n\r", signal, status);
                shell = 0;
            }
        }
        if ((pollfds[1].revents&POLLERR) == POLLERR) {
            fprintf(stderr, "pipe from shell has POLLERR\n\r");
            break;
        }
    }

    close(fdTtoS[1]);
    close(fdStoT[0]);
    fprintf(stderr, "reached end of program\n\r");
    resetTerminalMode();
    
    return 0;
}