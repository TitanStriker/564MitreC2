# https://www.geeksforgeeks.org/python/socket-programming-python/
import socket
import ssl
import random
import threading

host = '0.0.0.0'  # Open to anyone on the same wifi
port = 8888

types = ['HELO', 'EXIT', 'READ', 'RITE', 'CMD', 'ERR']

def parseAndSendInput(s: ssl.SSLSocket, user_input):
    user_input = user_input.split(' ')
    type = user_input[0]
    id = str(random.randint(0, 10 ** 9))
    data = " ".join(user_input[1:])
    assert type in types, f"unknown type {type} of length {len(type)}"
    if type in ['READ', 'RITE', 'CMD']:
        assert(data != '')
    else:
        assert(data == '')
    s.send(("  ".join([type, id, data])).encode())

def receiveMessage(c: ssl.SSLSocket):
    while True:
        data = c.recv(1024)
        if not data:
            print("Disconnected")
            break

        try:
            response = data.decode().replace('\\n', '\n')
        except Exception as e:
            print(f"[!] Error decoding response: {e}")
            break

        print(f"Received message: {response}")

# Create SSL context for the server
ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
ssl_context.load_cert_chain(certfile='cert.pem', keyfile='key.pem')

# Set up a socket and listen for a connection
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as raw_socket:
    raw_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    raw_socket.bind((host, port))
    raw_socket.listen()
    print(f"Listening on {host}:{port} (TLS)")

    with ssl_context.wrap_socket(raw_socket, server_side=True) as s:
        c, a = s.accept()

        # On connecting, use threading to both send and receive messages appropriately
        threading.Thread(target=receiveMessage, args=(c,), daemon=True).start()
        with c:
            print(f"Connected to {a}")
            while True:
                parseAndSendInput(c, input('> '))