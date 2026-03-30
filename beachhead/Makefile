# Change this to your url
URL ?= "Default Url Here"

# Compiler and flags for c++
CXX = g++
CXXFLAGS = -O2 -s -DURL='$(URL)'

TARGET = beachhead
SRC = beachhead.cpp 

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)