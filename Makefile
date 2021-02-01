CC=clang
COMMON= -Wall -Wextra -Wfloat-equal -pedantic -std=c90
DEBUG= -g3
SANITIZE= $(COMMON) -fsanitize=undefined -fsanitize=address $(DEBUG)
VALGRIND= $(COMMON) $(DEBUG)
PRODUCTION= $(COMMON) -O3
SDLCFLAGS=`sdl2-config --cflags`
SDLLIBS=`sdl2-config --libs`
LDLIBS = -lm

all : testparse testparse_s testparse_v testinterp testinterp_s testinterp_v testext testext_s testext_v

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

testinterp_v : interp.c
	$(CC) interp.c neillsdl2.c Stack/Linked/linked.c General/general.c -o interp_v $(VALGRIND) $(SDLCFLAGS) $(SDLLIBS) $(LDLIBS)

testext : extension.c
	$(CC) extension.c neillsdl2.c Stack/Linked/linked.c General/general.c -o ext $(PRODUCTION) $(SDLCFLAGS) $(SDLLIBS) $(LDLIBS)

testext_s : extension.c
	$(CC) extension.c neillsdl2.c Stack/Linked/linked.c General/general.c -o ext_s $(SANITIZE) $(SDLCFLAGS) $(SDLLIBS) $(LDLIBS)

testext_v : extension.c
	$(CC) extension.c neillsdl2.c Stack/Linked/linked.c General/general.c -o ext_v $(VALGRIND) $(SDLCFLAGS) $(SDLLIBS) $(LDLIBS)

clean:
	rm -f parse parse_s parse_v interp interp_s interp_v

run: all
	./parse GFX/rose.ttl
	./parse_s GFX/rose.ttl
	valgrind ./parse_v GFX/rose.ttl
	./interp GFX/tentacle.ttl
	./interp_s GFX/tentacle.ttl
	valgrind ./interp_v GFX/tentacle.ttl
	./ext GFX/umbrella.ttl
	./ext_s GFX/umbrella.ttl
	valgrind ./ext_v GFX/umbrella.ttl
	
