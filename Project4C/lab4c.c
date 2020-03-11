/*
NAME: Derek Vance
EMAIL: dvance@g.ucla.edu
ID: 604970765
*/

// THIS IS FOR LAB 4.

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>
#include <math.h>
#include <fcntl.h>
#include <poll.h>


const int B = 4275;
const int R0 = 100000;

sig_atomic_t volatile run_flag = 1;

void button_func() {
	run_flag = 0;
}

float CtoF(float C) {
	return C * 9 / 5 + 32;	
}

char* itos2(char* ret, int i) { // converts a two digit integer to a two character string
	ret[2] = '\0'; 
	ret[1] = '0' + (i % 10);
	if (i >= 0 && i < 10)
		ret[0] = '0';
	else if (i < 100) 
		ret[0] = '0' + (i / 10);
	else {
		fprintf(stderr, "itos2(int) received parameter greater than 100: %d\n", i);
		exit(1);
	}
	return ret;
}

int main(int argc, char **argv) {
	static struct option long_options[] = {
		{"period", required_argument, NULL, 1 },
		{"scale", required_argument, NULL, 2 },
		{"log", required_argument, NULL, 3 },
		{0, 0, 0, 0}
	};
	int c;
	int period = 1;
	char scale = 'F';
	int logfd = -1;
	while ((c = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
	switch(c) {
		case 1:
			period = atoi(optarg);
			break;
		case 2:
			if (strcmp(optarg, "F") == 0)
				scale = 'F';
			else if (strcmp(optarg, "C") == 0)
				scale = 'C';
			else {
				fprintf(stderr, "Unrecognized argument to --scale: %s\n", optarg);
				exit(1);
			}
			break;
		case 3:
			logfd = open(optarg, O_CREAT | O_WRONLY | O_TRUNC);
			break;
		default:
			fprintf(stderr, "Unrecognized option: %s\n", argv[optind - 1]);
			exit(1);		
	}
	}
	mraa_gpio_context button;
	mraa_aio_context tempsensor;
	tempsensor = mraa_aio_init(1);
	button = mraa_gpio_init(60);
	mraa_gpio_dir(button, MRAA_GPIO_IN);
	mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &button_func, NULL);
	signal(SIGINT, button_func);
	
	time_t rawtime;
	struct tm * timeinfo;
	int value, n;
	int sizes[2];
	sizes[0] = 0; sizes[1] = 0;
	char h[3], m[3], s[3], command[2][256], buffer[16];
	command[0][0] = '\0'; command[1][0] = '\0';
	struct pollfd pollfds[1] = {
		{STDIN_FILENO, POLLIN | POLLHUP | POLLERR, 0}
	};
	int scans = 0;
	int report = 1;
	struct timespec start, clock;
	clock_gettime(CLOCK_REALTIME_COARSE, &start);
	int seconds = 0;
	while (run_flag) {
		clock_gettime(CLOCK_REALTIME_COARSE, &clock);	
		int running = clock.tv_sec - start.tv_sec + (clock.tv_nsec - start.tv_nsec) / 1e9;
		if (running >= seconds) {
			if (report) {
				value = mraa_aio_read(tempsensor);
				float R = 1023.0/value - 1.0;
				R = R0 * R;
				float temperature = 1.0 / (log(R/R0) / B + 1/298.15) - 273.15;
				if (scale == 'F')
					temperature = CtoF(temperature);
				time(&rawtime);
				timeinfo = localtime(&rawtime);
				printf("%s:%s:%s %.1f\n", itos2(h,timeinfo->tm_hour), itos2(m,timeinfo->tm_min), itos2(s,timeinfo->tm_sec), temperature);
				if (logfd != -1)
				dprintf(logfd, "%s:%s:%s %.1f\n", itos2(h,timeinfo->tm_hour), itos2(m,timeinfo->tm_min), itos2(s,timeinfo->tm_sec), temperature);
			}
			seconds += period;
		}
		if ((poll(pollfds, 1, 0)) < 0) {
			fprintf(stderr, "ERROR polling\n");
			exit(1);
		}
		if (pollfds[0].revents&POLLIN) {
			n = read(STDIN_FILENO, buffer, 16);
			if (n < 0) {
				fprintf(stderr, "ERROR reading from stdin\n");
				exit(1);
			}
			int i;
			for (i = 0; i < n; i++) {
				if (buffer[i] == '\n') {
					command[scans][sizes[scans]] = '\0';
					sizes[scans] += 1;
					if (strcmp(command[0], "SCALE=") == 0) {
						if (strcmp(command[1], "F") == 0) {
							scale = 'F';
						}
						else if (strcmp(command[1], "C") == 0) {
							scale = 'C';
						}
						else {
							if (logfd != -1) {
								dprintf(logfd, "invalid stdin command: %s%s\n", command[0], command[1]);
							}
						}
					}
					else if (strcmp(command[0], "PERIOD=") == 0) {
						period = atoi(command[1]);
					}
					else if (strcmp(command[0], "LOG ") == 0) {
						
					}
					else if (strcmp(command[0], "START") == 0) {
						report = 1;
					}
					else if (strcmp(command[0], "STOP") == 0) {
						report = 0;
					}
					else if (strcmp(command[0], "OFF") == 0) {
						run_flag = 0;
					}
					else {
						if (logfd != -1) {
							dprintf(logfd, "Invalid stdin command: ");
						}	
					}
					if (logfd != -1) {
						dprintf(logfd, "%s%s\n", command[0], command[1]);
					}
					if (run_flag == 0) break;
					sizes[0] = 0; sizes[1] = 0;
					command[0][0] = '\0'; command[1][0] = '\0';
					scans = 0;
				}
				else if (buffer[i] == '=' || buffer[i] == ' ') {
					if (scans == 0) {
						command[scans][sizes[scans]] = buffer[i];
						command[scans][sizes[scans] + 1] = '\0';
						sizes[scans] += 2;
						scans = 1;
					}
					else {
						command[scans][sizes[scans]] = buffer[i];
						sizes[scans] += 1;
					}
				}
				else {
					command[scans][sizes[scans]] = buffer[i];
					sizes[scans] += 1;
				}
				if (sizes[scans] >= 256) {
					fprintf(stderr, "ERROR overflowing stdin buffer\n");
					exit(1);
				}
			}
			
		}
	}
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	printf("%s:%s:%s %s\n", itos2(h,timeinfo->tm_hour), itos2(m,timeinfo->tm_min), itos2(s,timeinfo->tm_sec), "SHUTDOWN");
	if (logfd != -1)
		dprintf(logfd, "%s:%s:%s %s\n", itos2(h,timeinfo->tm_hour), itos2(m,timeinfo->tm_min), itos2(s,timeinfo->tm_sec), "SHUTDOWN");
	mraa_gpio_close(button);
	mraa_aio_close(tempsensor);	
	return 0;
}
