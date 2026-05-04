# https://www.geeksforgeeks.org/python/socket-programming-python/
import socket
import ssl
import random
import threading

host = '0.0.0.0' # Open to anyone on the same wifi
port = 8888

types = ['HELO', 'EXIT', 'READ', 'RITE', 'CMD', 'ERR', 'RECON']

lock = threading.Lock()
addresses = [ ]

def parseAndSendInput(user_input):
    user_input = user_input.split(' ')

    try:
        type = user_input[0]

        # handle server exclusive commands
        if type is 'listconns':
            print("List of connections:\n")
            idx = 0
            for e in addresses:
                print(f"\t[{idx}]: {addr}", idx, addresses[0])
                idx += 1
            return

        if type is 'help':
            print("Usage: [server command] [connection id] [params...]"
                  "Server local commands: \'help\', \'listconns\' "
                  "Client affecting commands: \'HELO\', \'EXIT\', \'READ\', \'RITE\', \'CMD\', \'ERR\', \'RECON\'")
            return

        id = str(random.randint(0, 10 ** 9))
        addressIdx = user_input[1]
        connection = addresses[int(addressIdx)][1]

        # handle server -> client commands
        data = " ".join(user_input[2:])
        assert type in types, f"unknown type {type} of length {len(type)}"
        if type in ['READ', 'RITE', 'CMD']:
            assert(data != '')
        else:
            assert(data == '')

        connection.send(("  ".join([type, id, data])).encode())
    except:
        print("Something went wrong. Type 'help' for details on usage.")

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

        # While true, accept new connections and register them 
        threading.Thread(target=parseAndSendInput, args=(input('> ')), daemon=True).start()
        while True:
            c, a = s.accept()

            with c:
                threading.Thread(target=receiveMessage, args=(c,), daemon=True).start()
                print(f"Connected to {a}")
                with lock:
                    addresses.append((a, c))
