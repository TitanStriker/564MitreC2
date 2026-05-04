#!/bin/bash

# SSH Key Cracker Script

# Usage: ./ssh_key_cracker.sh <wordlist_path> <keys_directory>

# This script extracts hashes from SSH private keys and attempts to crack them using John the Ripper

# Check if correct number of arguments provided

if [ $# -ne 2 ]; then

        echo "Usage: $0 <wordlist_path> <keys_directory>"

        echo ""

        echo "Arguments:"

        echo " <wordlist_path> - Path to the wordlist file"

        echo " <keys_directory> - Path to directory containing SSH private keys"

        exit 1

fi

WORDLIST="$1"

KEYS_DIR="$2"

HASHES_FILE="ssh_hashes.txt"

JOHN_OUTPUT="john_results.txt"

JOHN_PATH="john_results.txt"

# Validate wordlist exists

if [ ! -f "$WORDLIST" ]; then

        echo "Error: Wordlist file not found: $WORDLIST"

        exit 1

fi

# Validate keys directory exists

if [ ! -d "$KEYS_DIR" ]; then

        echo "Error: Keys directory not found: $KEYS_DIR"

        exit 1

fi

# Check if ssh2john is available

if ! command -v ssh2john &>/dev/null; then

        echo "Error: ssh2john not found. Please install John the Ripper."

        exit 1

fi

# Check if john is available

# Point to your actual john binary
JOHN_PATH="$HOME/564/keystealer/john/run/john"

if [ ! -x "$JOHN_PATH" ]; then
    echo "Error: john binary not found at $JOHN_PATH"
    exit 1
fi

echo "[*] SSH Key Cracker Started"

echo "[*] Wordlist: $WORDLIST"

echo "[*] Keys Directory: $KEYS_DIR"

echo ""

# Clear/create hashes file

>"$HASHES_FILE"

# Process each private key file

echo "[*] Extracting hashes from SSH private keys..."

key_count=0

for key_file in "$KEYS_DIR"/*; do

        if [ -f "$key_file" ]; then

                # Check if it looks like a private key

                if grep -q "PRIVATE KEY" "$key_file" 2>/dev/null; then

                        echo "[+] Processing: $(basename "$key_file")"

                        ssh2john "$key_file" >>"$HASHES_FILE" 2>/dev/null

                        if [ $? -eq 0 ]; then

                                ((key_count++))

                        else

                                echo "[-] Failed to process: $(basename "$key_file")"

                        fi

                fi

        fi

done

echo ""

echo "[*] Processed $key_count SSH private keys"

echo "[*] Hashes stored in: $HASHES_FILE"

echo ""

# Check if we have any hashes

if [ ! -s "$HASHES_FILE" ]; then

        echo "[-] No hashes were extracted. Exiting."

        exit 1

fi

# Run John the Ripper

echo "[*] Running John the Ripper with wordlist..."

echo "[*] This may take a while..."

echo ""

"$JOHN_PATH" --wordlist="$WORDLIST" --format=ssh "$HASHES_FILE" | tee "$JOHN_OUTPUT"

echo ""

echo "[*] John the Ripper completed"

echo "[*] Results saved to: $JOHN_OUTPUT"

echo ""

echo "[*] To display cracked passwords, run:"

echo " $JOHN_PATH --show --format=ssh $HASHES_FILE"