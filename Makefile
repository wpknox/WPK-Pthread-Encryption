all: multi_encrypt.c
	gcc multi_encrypt.c -Wall -Werror -ggdb3 -O2 -o encrypt -lpthread
clean:
	rm -f encrypt *~
