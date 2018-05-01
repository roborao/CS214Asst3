all: netsv libf
netsv: netfileserver.c
	gcc -g -Wall -Werror  -pthread -c netfileserver.c
libf: libnetfiles.c
	gcc -g -Wall -o libnetfiles libnetfiles.c netfileserver.o

clean:
	rm -rf netfileserver.o libnetfiles
