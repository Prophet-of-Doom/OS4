CC = gcc

all: oss user

oss: oss.c header.h 
	gcc -g -Wall -lpthread -lrt -o oss oss.c

user: user.c header.h
	gcc -g -Wall -lpthread -lrt -o user user.c

clean: 
	$(RM) oss user
