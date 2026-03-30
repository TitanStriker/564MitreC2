# https://www.geeksforgeeks.org/python/socket-programming-python/
import socket             
import random          
import pwn  
import base64 

s = socket.socket()         

def enc(plaintext):
  return base64.encodebytes(pwn.xor(plaintext, key))

def dec(ciphertext):
  return pwn.xor(base64.decodebytes(ciphertext), key)

port = 8080
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
    parse(s, input('> '))
    print(dec(s.recv(4096)).decode().replace('\\n', '\n'))


s.close()