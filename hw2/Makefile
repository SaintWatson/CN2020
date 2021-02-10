all: 
	g++ -std=c++11 server.cpp common.cpp -o server
	g++ -std=c++11 client.cpp common.cpp -o client
	g++ player.cpp -o player `pkg-config --cflags --libs opencv`
clean:
	@rm ./server ./client ./player
serverTest:
	./server 3333
clientTest:
	./client 127.0.0.1:3333
