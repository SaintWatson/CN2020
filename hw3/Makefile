all:
	g++ -std=c++11 sender.cpp -o sender `pkg-config --cflags --libs opencv`
	g++ -std=c++11 receiver.cpp -o receiver `pkg-config --cflags --libs opencv`
	g++ -std=c++11 agent.cpp -o agent
	g++ -std=c++11 player.cpp -o player `pkg-config --cflags --libs opencv`
sender:
	g++ -std=c++11 sender.cpp -o sender `pkg-config --cflags --libs opencv`
receiver:
	g++ -std=c++11 receiver.cpp -o receiver `pkg-config --cflags --libs opencv`
agent:
	g++ -std=c++11 agent.cpp -o agent
player:
	g++ -std=c++11 player.cpp -o player `pkg-config --cflags --libs opencv`
clean:
	@rm sender receiver agent player
senderTest:
	./sender localhost 8878 8877 tmp.mpg
agentTest:
	./agent local local 8877 8878 8879 0.01
receiverTest:
	./receiver localhost 8878 8879
