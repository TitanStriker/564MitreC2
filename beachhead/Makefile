# ==============================================================================
# COMPSCI564 - Beachhead Makefile
# ==============================================================================
# This Makefile compiles the beachhead binary with minimal dependencies
# 
# Usage:
#   make                          # Build with default URL
#   make URL='"http://..."'       # Build with custom implant URL
#   make clean                    # Remove build artifacts
#   make debug                    # Build with debug symbols and logging
#
# AI Assistance Attribution:
# Build configuration developed with assistance from Claude (Anthropic)
# ==============================================================================

# Implant download URL (set via command line or use default)
URL ?= "http://10.37.1.249/implant"

# Compiler and flags
CXX = g++
CXXFLAGS = -O2 -s -Wall -Wextra -DURL='$(URL)'

# Linker flags
LDFLAGS =

# Target binary
TARGET = beachhead

# Source files
SRCS = beachhead.cpp privesc_check.cpp privesc.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# ==============================================================================
# Build Targets
# ==============================================================================

# Default target
all: $(TARGET)

# Link object files to create beachhead binary
$(TARGET): $(OBJS)
	@echo "[*] Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "[*] Stripping debug symbols..."
	strip $(TARGET)
	@echo "[+] Build complete: $(TARGET)"
	@ls -lh $(TARGET)

# Compile beachhead.cpp
beachhead.o: beachhead.cpp privesc_check.h privesc.h
	@echo "[*] Compiling beachhead.cpp..."
	$(CXX) $(CXXFLAGS) -c beachhead.cpp -o beachhead.o

# Compile privesc_check.cpp
privesc_check.o: privesc_check.cpp privesc_check.h
	@echo "[*] Compiling privesc_check.cpp..."
	$(CXX) $(CXXFLAGS) -c privesc_check.cpp -o privesc_check.o

# Compile privesc.cpp
privesc.o: privesc.cpp privesc.h
	@echo "[*] Compiling privesc.cpp..."
	$(CXX) $(CXXFLAGS) -c privesc.cpp -o privesc.o

# ==============================================================================
# Debug Build
# ==============================================================================

debug: CXXFLAGS = -O0 -g -Wall -Wextra -DDEBUG -DURL='$(URL)'
debug: clean $(TARGET)
	@echo "[+] Debug build complete (symbols preserved, logging enabled)"

# ==============================================================================
# Utility Targets
# ==============================================================================

# Clean build artifacts
clean:
	@echo "[*] Cleaning build artifacts..."
	rm -f $(TARGET) $(OBJS)
	@echo "[+] Clean complete"

# Show build configuration
info:
	@echo "===================================================================="
	@echo "Beachhead Build Configuration"
	@echo "===================================================================="
	@echo "Target:        $(TARGET)"
	@echo "Compiler:      $(CXX)"
	@echo "Flags:         $(CXXFLAGS)"
	@echo "Implant URL:   $(URL)"
	@echo "Source Files:  $(SRCS)"
	@echo "===================================================================="

# Test build (compile but don't strip)
test: CXXFLAGS = -O2 -Wall -Wextra -DDEBUG -DURL='$(URL)'
test: clean
	@echo "[*] Building test version (with debug output)..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)
	@echo "[+] Test build complete"
	@ls -lh $(TARGET)

# ==============================================================================
# Phony Targets
# ==============================================================================

.PHONY: all clean debug info test
