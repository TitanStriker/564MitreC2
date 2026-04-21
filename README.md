# CS564: Real World Group Final Project

## Project Overview
This project demonstrates how a full cyber-effect pipeline can be built against an Apache server vulnerable to CVE-2021-41773. This CVE allows for path traversal and RCE on an Apache server if cgi-bin scripts are enabled on the server. The vulnerability stems from failing to sanitize 
'.%2e/', more commonly seen as  ../, which navigates to a parent directory. Our project uses the exploit to download and run a beach head. This beach head first performs reconnaisance on the target for privilege escalation vectors, then it performs a privilege escalation to make the user a sudoer. Next, it downloads the C2 implant, adds persistence and runs it. The C2 implant will then receive commands from the C2 server, execute them, and return the outputs to the exfiltration server.
All communication is currently XOR encrypted, which serves as an obfuscation method more than offering any level of security. More robust cryptography and data exfiltration will be added for a later milestone.

## Target Setup Instructions
Refer to `./docs/env-setup-apache2_4_49.md` for how to set up the vulnerable target VM.

## Build/Run Instructions
To run this code:
```bash
sudo python3 trigger.py
```

This script runs all stages of our pipeline, including building and hosting the compiled files as well as launching the C2 server.  

## Threat-Model Diagram

![./docs/images/Diagram.drawio.png](./docs/images/Diagram.drawio.png)
