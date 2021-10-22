# defualts to building server obj file
all : server

# make run launches server with default params
run : server
	./server

server : server.c utils.c connect.c
	gcc -Wall -pthread $^ -o $@

clean:
	rm server
