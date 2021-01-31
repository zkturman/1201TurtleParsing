CC=clang
COMMON= -Wall -Wextra -Wfloat-equal -pedantic -std=c90
DEBUG= -g3
SANITIZE= $(COMMON) -fsanitize=undefined -fsanitize=address $(DEBUG)
VALGRIND= $(COMMON) $(DEBUG)
PRODUCTION= $(COMMON) -O3
SDLCFLAGS=`sdl2-config --cflags`
SDLLIBS=`sdl2-config --libs`
LDLIBS = -lm

testparse : parse.c
	$(CC) parse.c -o parse $(PRODUCTION) $(LDLIBS)

testparse_s : parse.c
	$(CC) parse.c -o parse_s $(SANITIZE) $(LDLIBS)

testparse_v : parse.c
	$(CC) parse.c -o parse_v $(VALGRIND) $(LDLIBS)

testinterp : interp.c
	$(CC) interp.c neillsdl2.c Stack/Linked/linked.c General/general.c -o interp $(PRODUCTION) $(SDLCFLAGS) $(SDLLIBS) $(LDLIBS)

testinterp_s : interp.c
	$(CC) interp.c neillsdl2.c Stack/Linked/linked.c General/general.c -o interp_s $(SANITIZE) $(SDLCFLAGS) $(SDLLIBS) $(LDLIBS)

testfull : fullinterp.c
	$(CC) fullinterp.c neillsdl2.c Stack/Linked/linked.c General/general.c -o full $(PRODUCTION) $(SDLCFLAGS) $(SDLLIBS) $(LDLIBS)

testfull_s : fullinterp.c
	$(CC) fullinterp.c neillsdl2.c Stack/Linked/linked.c General/general.c -o full_s $(SANITIZE) $(SDLCFLAGS) $(SDLLIBS) $(LDLIBS)

clean:
	rm -f parse parse_s parse_v interp interp_s interp_v

basic: parse_s parse_v
	./parse_s
	valgrind ./parse_v
