CC      =		cc
CFLAGS  =		-O3 -pipe -D_FORTIFY_SOURCE=3 -D_GLIBCXX_ASSERTIONS \
				-fstack-protector-strong -fstack-clash-protection -fcf-protection \
				-fpie -fpic
LDFLAGS	=		-Wl,-z,now -Wl,-z,relro

CODE_FILES =	src/hack_assembler.c src/compiler/create_hashmaps.c src/helpers.c src/compiler/parser.c src/compiler/compiler.c
COMMAND	=		${CC} ${CFLAGS} ${LDFLAGS}

release: create_builds_dir compile_dynamic_libraries src/hack_assembler.c
	${COMMAND} -Wl,-rpath,'$$ORIGIN' -L./builds -lhashmap -larraylist ${CODE_FILES} -o builds/hack_assembler

compile_dynamic_libraries: create_builds_dir
	# Compile hashmap.c
	${COMMAND} -shared -c hashmap.c/hashmap.c -o builds/libhashmap.so

	# Compile c-array-list
	cd c-array-list && make
	cd builds && ln -sf ../c-array-list/builds/libarraylist.so libarraylist.so

test: release src/hack_assembler.c test/asm/Add.asm
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all ./builds/hack_assembler -fi ./test/asm/Max.asm -o ./test/to_compare/Max.hack

clean:
	rm builds/hack_assembler

create_builds_dir:
	@if [ ! -d "builds" ]; then\
        mkdir builds;\
    fi
