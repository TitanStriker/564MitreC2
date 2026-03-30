# Change this to your url
URL ?= "http://10.37.1.249/implant.py"

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