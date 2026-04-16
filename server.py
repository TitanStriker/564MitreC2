# https://www.geeksforgeeks.org/python/socket-programming-python/
import socket             
import random      
import threading    
# import pwn  
import base64 

host = '0.0.0.0' # Open to anyone on the same wifi
port = 8888

key = b"ED IS COOL"
    
def fixed_xor(arg1: bytes, arg2: bytes) -> bytes:
    assert len(arg1) == len(arg2), "Trying to xor mismatched lengths!"
    return bytes([arg1[i] ^ arg2[i] for i in range(len(arg1))])
  
def xor(message: bytes, key: bytes) -> bytes:
    """XOR a message with a repeating key.

    Works for messages of any length, including empty messages, without
    asserting on the length. This avoids crashes when the socket returns
    zero bytes (peer closed the connection).
    """
    if not message:
        return b""
    ret = []
    for i in range(len(message)):
        ret.append(message[i] ^ key[i % len(key)])
    return bytes(ret)

def enc(plaintext):
  return base64.encodebytes(xor(plaintext, key))

def dec(ciphertext):
  return xor(base64.decodebytes(ciphertext), key)

types = ['HELO', 'EXIT', 'READ', 'RITE', 'CMD', 'ERR']

def parseAndSendInput(s: socket.socket, user_input):
    user_input = user_input.split(' ')
    type = user_input[0]
    id = str(random.randint(0, 10 ** 9))
    data = " ".join(user_input[1:])
    assert type in types, f"unknown type {type} of length {len(type)}"
    if type in ['READ', 'RITE', 'CMD']:
        assert(data != '')
    else:
        assert(data == '')
    s.send(enc(("  ".join([type, id, data])).encode()))

def receiveMessage(c):
   while True:
       data = c.recv(1024)
       if not data:
           print("Disconnected")
           break

       try:
           response = dec(ciphertext).decode().replace('\\n', '\n')
       except Exception as e:
           print(f"[!] Error decoding response: {e}")
           break

       print(f"Received message: {response}")

# Set up a socket and listen for a connection
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
   s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
   s.bind((host, port))
   s.listen()
   print(f"Listening on {host}:{port}")
   c, a = s.accept()

   # On connecting, use threading to both send and receive messages appropriately
   threading.Thread(target=receiveMessage, args=(c,), daemon=True).start()
   with c:
       print(f"Connected to {a}")
       while True:
           parseAndSendInput(s, input('> '))

s.close()