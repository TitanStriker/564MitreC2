# CS564: Real World Group Final Project

## Project Overview
This project demonstrates how a full cyber-effect pipeline can be built against an Apache server vulnerable to CVE-2021-41773. This CVE allows for path traversal and RCE on an Apache server if cgi-bin scripts are enabled on the server. The vulnerability stems from failing to sanitize 
'.%2e/', more commonly seen as  ../, which navigates to a parent directory. Our project uses the exploit to download and run a beach head. This beach head first performs reconnaisance on the target for privilege escalation vectors, then it performs a privilege escalation to make the user a sudoer. Next, it downloads the C2 implant, adds persistence and runs it. The C2 implant will then receive commands from the C2 server, execute them, and return the outputs to the exfiltration server.
All communication to and from the target machine is encrypted.

## Target Setup Instructions
Refer to `./docs/env-setup-apache2_4_49.md` for how to set up the vulnerable target VM.

## Build/Run Instructions
To run this code:
```bash
sudo python3 trigger.py
```

This script runs all stages of our pipeline, including building and hosting the compiled files as well as launching the C2 server.  

## Architecture
The architecture for this project is as follows: 2 servers are ran by the attacker and a c2 implant runs on the target machine which communicates with both attacker servers. One server is the c2 server which allows the attacker to run commands on the target machine via a command line interface. The other server is the exfiltration server which receives data from the implant. All traffic between these systems is encrypted.  

In order to deploy the implant on a target machine, the exploit chain must be invoked on a vulnerable target. This invokes our two stage exploit chain: exploit -> beachhead -> C2 implant. After the beachhead is downloaded and ran, it performs reconnaisance on the machine and looks for further vulnerabilities. If possible, priviledge escalation is performed to establish persistance for the c2 implant. If not, then the c2 implant is ran normally. The c2 implant and beachhead binaries are implemented with c++ and are stripped. Additionally, unsuspecting filenames are locations are used for the persistent files. The c2 implant is capable of running arbitary commands from the server and carrying out reconnaisance.  

## Implant Built-in Tasks
1. **RECON**: Performs comprehensive system reconnaissance. This includes gathering OS version, kernel information, UID/GID (checking for root), and hardware/virtualization details. It also enumerates network interfaces, lists the top processes, detects security products, and collects sensitive files like `/etc/passwd`, `/etc/shadow`, and SSH authorized keys.
2. **IP_REPORT**: Provides network-centric intelligence. It uses external services (`ipinfo.io`) to determine the public IP and geographic location, and local tools (`ifconfig`, `netstat`) to map internal network interfaces and routing tables.
3. **ESCAPE**: Designed for container environments. It detects if the implant is running within a Docker container and assesses if it has "privileged" capabilities. If conditions are met, it attempts an LVM mount escape to break out of the container and gain access to the host filesystem.
4. **Key Cracking**: Facilitated through the **RECON** task, which automatically exfiltrates `/etc/shadow` and SSH `authorized_keys`. This provides the necessary data for offline password cracking and lateral movement.

## Privilege Escalation
The beachhead module includes a "Tar Wildcard Privilege Escalation" exploit. It specifically targets systems where a root process (often via cron) periodically runs `tar` with a wildcard (`*`) in a directory that a lower-privileged user can write to. By creating files with names that match `tar` command-line flags (e.g., `--checkpoint=1` and `--checkpoint-action=exec=sh exploit.sh`), the attacker can trick `tar` into executing an arbitrary script as the root user. In this project, the exploit is used to add the `daemon` user to the sudoers file, granting full root access.

## Self-Destruct
The `SELF_DESTRUCT` command is the ultimate cleanup mechanism. When triggered, the implant:
1. Stops and disables any persistence services it established (e.g., systemd units).
2. Deletes all temporary files, artifacts, and tools used during the exploit chain (located in `/tmp`, `/var/tmp`, etc.).
3. Clears system logs (like `auth.log`, `syslog`, `apache2/access.log`) and shell history to hide its tracks.
4. Removes any privilege escalation markers (e.g., custom sudoers entries).
5. Terminates all associated processes and deletes its own executable from the disk.

## Detection (YARA Rules)
To support defensive research and incident response, this project includes a set of YARA rules located in the `yara/` directory. These rules are designed to detect various components of the attack chain:

1. **C2_Beachhead**: Identifies the first-stage loader by looking for status logs (e.g., "Checking privilege escalation conditions"), temporary filenames (e.g., `systemd-private-update`), and specific shell commands used for initialization.
2. **C2_Implant**: Targets the primary C2 binary by detecting unique reporting strings (e.g., "=== FULL RECON REPORT ==="), container escape markers, and obfuscated paths used during the LVM breakout.
3. **C2_Shared_Logic**: Detects common patterns shared across components, such as reconnaissance headers, cron-based persistence checks, and the standard set of C2 command keywords (`HELO`, `RECON`, `ESCAPE`, etc.).
4. **C2_Python_Server**: Identifies the attacker's infrastructure by matching function names and configuration strings specific to the C2 and exfiltration server implementation.

These rules can be used with the `yara` CLI tool to scan processes or files on a suspected system.
