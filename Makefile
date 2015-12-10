#Compiler
CC=g++

#FLAGS
CFLAGS= -std=c++11

#LIBRARIES
CLIBS = -lsensors

PROG=SafeTemp
TARGET=tempsafe

$(TARGET):$(PROG).cpp
	$(CC) $(CFLAGS) -o $(TARGET) $(CLIBS) $(PROG).cpp 
clean:
	$(RM) $(TARGET)
