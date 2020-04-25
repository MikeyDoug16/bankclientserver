CXXFLAGS=-std=c++14 -Werror -Wextra -Wall -Wwrite-strings -Wpedantic -pthread
all: client server cache

client: client.cpp
	g++ $(CXXFLAGS) client.cpp -o client

server: server.cpp
	g++ $(CXXFLAGS) server.cpp -o server
cache: lrucache.cpp
	g++ $(CXXFLAGS) lrucache.cpp -o cache


