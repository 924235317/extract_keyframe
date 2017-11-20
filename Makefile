CC=g++ -w -g -std=c++11

LIBS+=`pkg-config --libs libavformat`
LIBS+=`pkg-config --libs libavcodec`
LIBS+=`pkg-config --libs libavutil`
LIBS+=`pkg-config --libs libswscale`
LIBS+=`pkg-config --libs opencv`
LIBS+=-pthread

FLAGS+=`pkg-config --cflags libavformat`
FLAGS+=`pkg-config --cflags opencv`

SOURCE=main.cpp extract_frame.cpp
OBJ=$(SOURCE:.cpp=.o)
EXE=bin

all:$(OBJ)
	$(CC) $(OBJ) -o $(EXE) $(LIBS)

extract_frame.o:
	$(CC) -c extract_frame.cpp $(FLAGS)
main.o:
	$(CC) -c main.cpp $(FLAGS)


clean:
	rm -rf $(OBJ)
	rm -rf $(EXE)
