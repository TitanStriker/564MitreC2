FROM python:3.11-slim

WORKDIR /app
COPY server.py .
COPY requirements.txt .

# Install dependencies
RUN pip install -r requirements.txt

EXPOSE 80

CMD ["python", "server.py"]
