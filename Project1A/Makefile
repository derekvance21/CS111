# NAME: Derek Vance
# EMAIL: dvance@g.ucla.edu
# ID: 604970765

TAR_NAME = lab1a-604970765.tar.gz
FILES_TO_TAR = lab1.c Makefile README

default: lab1.c
	gcc -g lab1.c -o lab1a -Wall -Wextra

clean:
	rm -f lab1a 
	rm -f $(TAR_NAME)

dist: lab1.c
	tar -czf $(TAR_NAME) $(FILES_TO_TAR)
