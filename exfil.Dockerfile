FROM python:3.9-slim

WORKDIR /app

RUN apt-get update && apt-get install -y openssl

COPY exfil_cert.pem .
COPY exfil_key.pem .

RUN touch /app/c2_outputs.txt

CMD ["sh", "-c", "openssl s_server -accept 443 -cert exfil_cert.pem -key exfil_key.pem >> /app/c2_outputs.txt"]
