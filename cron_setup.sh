#!/bin/bash

(crontab -l 2>/dev/null; echo "* * * * * cd /tmp/backups && for i in {1..12}; do tar -cf /tmp/backup.tar *; sleep 5; done") | crontab -