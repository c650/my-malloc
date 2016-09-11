CFLAGS = -Wall -Werror

# Sources for bot.o
SRC= ./src/*.c
OUT= ./bin/my-malloc.o

build: ${SRC}
	${CC} ${CFLAGS} -o ${OUT} ${SRC}

.PHONY: clean

clean:
	rm ./bin/my-malloc.o