# NAME: Derek Vance
# EMAIL: dvance@g.ucla.edu
# ID: 604970765

TAR_NAME = lab1b-604970765.tar.gz
FILES_TO_TAR = lab1b-server.c lab1b-client.c Makefile README

default: lab1b-server.c lab1b-client.c
	gcc lab1b-server.c -o lab1b-server -lz -Wall -Wextra
	gcc lab1b-client.c -o lab1b-client -lz -Wall -Wextra

server: lab1b-server.c
	gcc lab1b-server.c -o lab1b-server -lz -Wall -Wextra

client: lab1b-client.c
	gcc lab1b-client.c -o lab1b-client -lz -Wall -Wextra
	
clean:
	rm -f lab1b-client
	rm -f lab1b-server
	rm -f $(TAR_NAME)

dist: $(FILES_TO_TAR)
	tar -czf $(TAR_NAME) $(FILES_TO_TAR)