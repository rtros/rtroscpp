CC=g++

NAME=test_all

CFLAGS= -I/usr/realtime/include -I rtai_thread -o

DFLAGS= -L/usr/realtime/lib

TARGET=$(NAME)

SOURCE=$(NAME).cpp

all:
	$(CC) $(SOURCE) $(CFLAGS) $(TARGET) $(DFLAGS)
clean:
	rm $(TARGET)