all: netsv libf
netsv: netfileserver.c
	gcc -g -Wall -Werror -lpthread -o netfileserver.o netfileserver.c
libf: libnetfiles.c
	gcc -g -Wall -o libnetfiles.o libnetfiles.c 

clean:
	rm -rf netfileserver.o libnetfiles.o
