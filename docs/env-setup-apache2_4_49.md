# Locate the Exploit
https://www.exploit-db.com/exploits/50383

Install Ubuntu 24.04
Install the vulnerable Apache version from ExploitDB
```bash
sudo apt update 
sudo apt install gcc build-essential libpcre3 libpcre3-dev libapr1-dev libaprutil1-dev
cd ~
wget https://www.exploit-db.com/apps/1edb1895661473ea530209e29b83a982-httpd-2.4.49.tar.gz
tar xvf 1edb1895661473ea530209e29b83a982-httpd-2.4.49.tar.gz
cd ~/httpd-2.4.49/srclib

wget https://downloads.apache.org/apr/apr-1.7.6.tar.gz
tar xvf apr-1.7.6.tar.gz
mv apr-1.7.6 apr

wget https://downloads.apache.org/apr/apr-util-1.6.3.tar.gz
tar xvf apr-util-1.6.3.tar.gz
mv apr-util-1.6.3 apr-util

cd ~/Downloads/httpd-2.4.49

./configure --prefix=/usr/local/apache2 \
            --enable-so \
            --enable-mods-shared=all \
            --with-mpm=event \
            --with-included-apr \
            --disable-ssl
            

make -j$(nproc) 
sudo make install 

sudo vi /usr/local/apache2/conf/httpd.conf
```
Add the following lines to `/usr/local/apache2/conf/httpd.conf`
```
ServerName localhost
LoadModule cgid_module modules/mod_cgid.so

# change the preexisting root rule to
<Directory />
    Options FollowSymLinks
    AllowOverride None
    Require all granted
</Directory>
```
Check logs:
```bash
sudo /usr/local/apache2/bin/apachectl start
sudo /usr/local/apache2/bin/apachectl configtest
cat /usr/local/apache2/logs/error_log
```

Images:
![Pasted image 20260319215216.png](images/Pasted%20image%2020260319215216.png)
![Pasted image 20260319223921.png](images/Pasted%20image%2020260319223921.png)

FINAL MILESTONE UPDATES:

For the final milestone, we wanted to make a subroutine to perform a Docker Escape. This functionality 
requires us to change the setup slightly. We set up our Apache 2.4.49 version inside of the Docker Container
on our target. We also set up a mounted file system that we use to escape the Docker container.

The `docker-compose.yml` file for our target.
```yaml
services:
  vulnerable-apache:
    build: .
    container_name: apache-lab
    restart: always
    privileged: true
    ports:
      - "80:80"
```

The `Dockerfile` for our target.
```
# Stage 1: Build the vulnerable Apache on Ubuntu 24.04
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    wget build-essential libpcre3-dev libapr1-dev libaprutil1-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp
RUN wget https://archive.apache.org/dist/httpd/httpd-2.4.49.tar.gz && \
    tar -xf httpd-2.4.49.tar.gz

WORKDIR /tmp/httpd-2.4.49
RUN ./configure --prefix=/usr/local/apache2 --enable-cgi && \
    make -j$(nproc) && make install

# Stage 2: Final Research Image
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# 1. Install research tools + runtime dependencies
RUN apt-get update && apt-get install -y \
    cron openssl curl net-tools iputils-ping vim procps wget sudo \
    libapr1 libaprutil1 libpcre3 libssl3 lvm2 \
    && rm -rf /var/lib/apt/lists/*

# 2. Copy the compiled Apache from the builder stage
COPY --from=builder /usr/local/apache2 /usr/local/apache2

# 3. Create the lab environment (Scripts and Backups)
# Note: Ensure these files are in your local folder!
COPY setup.sh /root/setup.sh
COPY cleanup.sh /root/cleanup.sh
RUN chmod +x /root/setup.sh /root/cleanup.sh && \
    mkdir -p /tmp/backups && \
    chmod 777 /tmp/backups

# 4. Apply the CVE-2021-41773 vulnerability
WORKDIR /usr/local/apache2
RUN sed -i 's/#LoadModule cgid_module/LoadModule cgid_module/g' conf/httpd.conf && \
    sed -i 's/AllowOverride None/AllowOverride None\n    Require all granted/g' conf/httpd.conf && \
    sed -i 's/<Directory \/>/<Directory \/>\n    AllowOverride None\n    Require all granted/g' conf/httpd.conf

# 5. Set Path
ENV PATH="/usr/local/apache2/bin:$PATH"

# Start services (Using the direct binary instead of httpd-foreground)
CMD ["/bin/sh", "-c", "cron && httpd -D FOREGROUND"]
```
