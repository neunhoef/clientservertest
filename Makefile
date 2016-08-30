all: client server

client: client.cpp
	g++ client.cpp -o client -Wall -std=c++11 -g -O0

server: server.cpp
	g++ server.cpp -o server -Wall -std=c++11 -g -O0 -lpthread
