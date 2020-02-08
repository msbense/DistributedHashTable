all: node app

node: node.cpp
	g++ node.cpp tcp/tcp_connection.cpp -O3 -o ./obj64/node.o -lboost_system -lboost_thread -lpthread

app: app.cpp
	g++ app.cpp -O3 -o ./obj64/app.o -lboost_system -lboost_thread -lpthread

clean:
	rm -rf ./obj64/*