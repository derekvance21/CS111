# NAME: Derek Vance
# UID: 604970765

default:
	gcc -lm -lmraa -Wall -Wextra -o lab4b lab4b.c

check: default
	echo -e "STOP\nSTART\nOFF\n" | ./lab4b --scale=C --period=2 --log=loggg.txt
	rm -f loggg.txt
	
clean:
	rm -f lab4b lab4b-604970765.tar.gz

dist:
	tar -czf lab4b-604970765.tar.gz lab4b.c Makefile README 
