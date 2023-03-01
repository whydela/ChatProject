all: device server

server: Server.c
	gcc -Wall Server.c -o Server -lpthread

device: Device.c
	gcc -Wall Device.c -o Device

reset:
	rm *o Device Server
