# Change this to your url
URL ?= "http://10.37.1.249/implant.py"

# Compiler and flags for c++
CXX = g++
CXXFLAGS = -O2 -s -Wall -DURL='$(URL)'

TARGET = beachhead
SRCS = beachhead.cpp privesc_check.cpp privesc.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)
	strip $(TARGET)

beachhead.o: beachhead.cpp privesc_check.h privesc.h
	$(CXX) $(CXXFLAGS) -c beachhead.cpp

privesc_check.o: privesc_check.cpp privesc_check.h
	$(CXX) $(CXXFLAGS) -c privesc_check.cpp

privesc.o: privesc.cpp privesc.h
	$(CXX) $(CXXFLAGS) -c privesc.cpp

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
