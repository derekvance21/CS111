# NAME: Derek Vance
# EMAIL: dvance@g.ucla.edu
# UID: 604970765

FILES_TO_TAR = lab2_add.c SortedList.h SortedList.c lab2_list.c README Makefile lab2_add.gp lab2_list.gp tests.sh
TARBALL_NAME = lab2a-604970765.tar.gz
GRAPHS_ADD = lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png
GRAPHS_LIST = lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png

build: list add

add: lab2_add.c
	gcc -lpthread -Wall -Wextra -o lab2_add lab2_add.c

list: lab2_list.c SortedList.c
	gcc -lpthread -Wall -Wextra -o lab2_list lab2_list.c SortedList.c

tests: build
	rm -f lab2_add.csv lab2_list.csv
	chmod +x tests.sh
	-./tests.sh 2> /dev/null

graphs: tests
	chmod +x lab2_add.gp
	chmod +x lab2_list.gp
	./lab2_add.gp
	./lab2_list.gp

dist: graphs
	tar -czf $(TARBALL_NAME) $(FILES_TO_TAR) $(GRAPHS_ADD) $(GRAPHS_LIST) *.csv

clean_csv:
	rm -f *.csv

clean_png:
	rm -f *.png

clean_all: clean clean_csv clean_png

clean: 
	rm -f $(TARBALL_NAME)
	rm -f lab2_add
	rm -f lab2_list