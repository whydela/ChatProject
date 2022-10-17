all: Device Server

server: Server.c
	gcc -Wall Server.c -o Server

device: Device.c
	gcc -Wall Device.c -o Device

reset:
	rm *o Device Server
