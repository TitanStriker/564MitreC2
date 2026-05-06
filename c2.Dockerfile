FROM python:3.9-slim

WORKDIR /app

COPY *.pem server.py ./

CMD ["python", "-u", "server.py"]
