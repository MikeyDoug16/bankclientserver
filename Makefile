CXXFLAGS=-std=c++11 -Werror -Wextra -Wall -Wwrite-strings -Wpedantic
all: client server cache

client: client.cpp
	g++ $(CXXFLAGS) client.cpp -o client

server: server.cpp
	g++ $(CXXFLAGS) server.cpp -o server
cache: lrucache.cpp
	g++ $(CXXFLAGS) lrucache.cpp -o cache


