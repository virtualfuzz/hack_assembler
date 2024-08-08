CC      =	cc
CFLAGS  =	-Werror -Wall -Wextra -O3 -pipe -D_FORTIFY_SOURCE=3 -D_GLIBCXX_ASSERTIONS \
			-fstack-protector-strong -fstack-clash-protection -fcf-protection \
			-fpie -fpic
LDFLAGS	=	-Wl,-z,now -Wl,-z,relro

release: src/hack_assembler.c
	${CC} ${CFLAGS} ${LDFLAGS} src/hack_assembler.c -o builds/hack_assembler

clean:
	rm hack_assembler