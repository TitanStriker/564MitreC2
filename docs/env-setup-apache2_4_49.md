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

