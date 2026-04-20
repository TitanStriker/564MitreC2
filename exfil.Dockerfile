FROM python:3.9-slim

WORKDIR /app

COPY exfil.py .

CMD ["python", "-u", "exfil.py"]