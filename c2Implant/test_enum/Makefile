CXX = g++
CXXFLAGS = -O2 -Wall
TARGET = test_env

SRC = main.cpp docker_enum.cpp
INC = -I.

all: $(TARGET)

$(TARGET): $(SRC) docker_enum.h
	$(CXX) $(CXXFLAGS) $(INC) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
