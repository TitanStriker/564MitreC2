# exfil.py
import socket
import threading

host = '0.0.0.0' # Open to anyone on the same wifi, i think more than this requires port forwarding
port = 8889

# convert from a bytes object to plain text
def decrypt(b):
    return b.decode().replace('\\n', '\n');

def receiveMessage(a, c):
   with c:
        print(f"Connected to {a}")
        while True:
            data = c.recv(1024)
            if not data:
                print("Disconnected")
                break

            try:
                response = decrypt(data);
            except Exception as e:
                print(f"[!] Error decoding response: {e}")
                break

            print(f"Received message: {response}")
    


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