# Set the server ip and port
IP ?= "10.37.1.249"
PORT ?= 8888

# Compiler and flags for c++
CXX = g++
CXXFLAGS = -O2 -s -DIP='$(IP)' -DPORT='$(PORT)'

TARGET = implant
SRC = implant.cpp 

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
