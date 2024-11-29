CC      =	cc
CFLAGS  =	-Werror -Wall -Wextra -O3 -pipe -D_FORTIFY_SOURCE=3 -D_GLIBCXX_ASSERTIONS \
			-fstack-protector-strong -fstack-clash-protection -fcf-protection \
			-fpie -fpic
LDFLAGS	=	-Wl,-z,now -Wl,-z,relro

COMMAND	=	${CC} ${CFLAGS} ${LDFLAGS}

release: create_builds_dir src/hack_assembler.c
	${COMMAND} src/hack_assembler.c src/compiler/create_hashmaps.c hashmap.c/hashmap.c src/helpers.c src/compiler/parser.c src/compiler/compiler.c -o builds/hack_assembler

test: release src/hack_assembler.c test/asm/Add.asm
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all ./builds/hack_assembler -i ./test/asm/Add.asm -f

clean:
	rm builds/hack_assembler

create_builds_dir:
	@if [ ! -d "builds" ]; then\
        mkdir builds;\
    fi