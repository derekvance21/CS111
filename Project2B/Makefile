# NAME: Derek Vance
# EMAIL: dvance@g.ucla.edu
# UID: 604970765

TARBALL_NAME=lab2b-604970765.tar.gz

default: list

list: lab2_list.c SortedList.c
	gcc -lprofiler -lpthread -Wall -Wextra -o lab2_list lab2_list.c SortedList.c

profile: default
	rm -f profile.out
	CPUPROFILE=./lab.gperf ./lab2_list --iterations=1000 --threads=12 --sync=s
	pprof --text ./lab2_list ./lab.gperf > profile.out
	pprof --list=thread_func_s ./lab2_list ./lab.gperf >> profile.out
	rm -f lab.gperf

tests: default
	rm -f *.csv
	chmod +x tests.sh
	-./tests.sh 2> /dev/null

graphs: tests
	chmod +x lab2b_list.gp
	./lab2b_list.gp

dist: graphs profile
	tar -czf $(TARBALL_NAME) lab2b_list.csv *.png *.c *.h profile.out tests.sh lab2b_list.gp README Makefile

clean: 
	rm -f $(TARBALL_NAME)
	rm -f lab2_list