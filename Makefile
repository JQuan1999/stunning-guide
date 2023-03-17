CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = main
OBJS = main.cpp ./server/server.cpp ./server/m_epoll.cpp ./log/log.cpp ./http_conn/http_conn.cpp \
		./http_conn/http_request.cpp ./http_conn/http_response.cpp ./buffer/buffer.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET) -lpthread

clean:
	rm -rf $(TARGET)
