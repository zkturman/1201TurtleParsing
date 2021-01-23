CC=gcc
COMMON= -Wall -Wextra -Wfloat-equal -pedantic -std=c90
DEBUG= -g3
SANITIZE= $(COMMON) -fsanitize=undefined -fsanitize=address $(DEBUG)
VALGRIND= $(COMMON) $(DEBUG)
PRODUCTION= $(COMMON) -O3
LDLIBS = -lm

testparse : parse.c
	$(CC) parse.c -o parse $(PRODUCTION) $(LDLIBS)

testparse_s : parse.c
	$(CC) parse.c -o parse_s $(SANITIZE) $(LDLIBS)

testparse_v : parse.c
	$(CC) parse.c -o parse_v $(VALGRIND) $(LDLIBS)

clean:
	rm -f parse_s parse_v parse testcuckoo_s testcuckoo_v testcuckoo

basic: parse_s parse_v
	./parse_s
	valgrind ./parse_v
