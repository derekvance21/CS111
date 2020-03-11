# NAME: Derek Vance
# EMAIL: dvance@g.ucla.edu
# ID: 604970765

TAR_NAME = lab0-604970765.tar.gz
FILES_TO_TAR = lab0.c Makefile README backtrace.png breakpoint.png

.SILENT:clean
.SILENT:segfault_check
.SILENT:input_check
.SILENT:input_output_check
.SILENT:output_check
.SILENT:option_check
.SILENT:check
.SILENT:default

default: lab0.c
	gcc -g lab0.c -o lab0 -Wall -Wextra

segfault_check:
	touch log.txt; \
	./lab0 --catch --segfault 2> error.txt; \
	if [ $$? -ne 4 ]; then \
		echo "--catch --segfault did not return correct exit code of 4" >> log.txt; \
	fi; \
	if [ ! -s error.txt ]; then \
		echo "--catch --segfault did not write to stderr" >> log.txt; \
	fi; \
	> error.txt;

input_check:
	./lab0 --input="fake.txt" 2> error.txt; \
	if [ $$? -ne 2 ]; then \
		echo "--input with nonexistent file did not return correct exit code of 2" >> log.txt; \
	fi; \
	if [ ! -s error.txt ]; then \
		echo "--input with nonexistent file did not write to stderr" >> log.txt; \
	fi; \
	> error.txt;

input_output_check:
	echo "hello" > input.txt; \
	./lab0 --input="input.txt" --output="output.txt"; \
	if [ $$? -ne 0 ]; then \
		echo "valid input and output did not return correct exit code of 0" >> log.txt; \
	fi; \
	cmp --silent input.txt output.txt || echo "input and output are different" >> log.txt; \
	rm input.txt; \
	rm output.txt;

output_check:
	touch hi.txt; \
	chmod -w hi.txt; \
	echo "hi" | ./lab0 --output="hi.txt" 2> error.txt; \
	if [ $$? -ne 3 ]; then \
		echo "--output with invalid file did not return correct exit code of 3" >> log.txt; \
	fi; \
	if [ ! -s error.txt ]; then \
		echo "--output with invalid file did not write to stderr" >> log.txt; \
	fi; \
	rm -f hi.txt; \
	> error.txt;
	
option_check:
	./lab0 --badoption 2> error.txt; \
	if [ $$? -ne 1 ]; then \
		echo "usage of unrecognized option doesn't return exit code of 1" >> log.txt; \
	fi; \
	if [ ! -s error.txt ]; then \
		echo "usage of unrecognized option doesn't write to stderr" >> log.txt; \
	fi; \
	> error.txt;
	./lab0 badargument 2> error.txt; \
	if [ $$? -ne 1 ]; then \
		echo "usage of non-option argument doesn't return exit code of 1" >> log.txt; \
	fi; \
	if [ ! -s error.txt ]; then \
		echo "usage of unrecognized option doesn't write to stderr" >> log.txt; \
	fi; \
	> error.txt;

check: default segfault_check input_check input_output_check output_check option_check
	if [ -s log.txt ]; then \
		echo "tests failed for the following reasons:"; \
		cat log.txt; \
	else \
		echo "all tests passed!"; \
	fi; \
	rm log.txt; \
	rm error.txt;

clean:
	rm -f lab0
	rm -f $(TAR_NAME)

dist: $(FILES_TO_TAR)
	tar -czf $(TAR_NAME) $(FILES_TO_TAR)
