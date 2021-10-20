# defualts to building server obj file
all : server.o

# make run launches server with default params
run : server.o
	./server.o

server.o : server.c utils.c
	gcc -pthread $^ -o $@

clean:
	find . -name \*.o -type f -delete
