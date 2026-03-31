# CS564: Real World Group Final Project

## Project Overview
This project demonstrates how a full cyber-effect pipeline can be built against an Apache server vulnerable to CVE-2021-41773. This CVE allows for path traversal and RCE on an Apache server if cgi-bin scripts are enabled on the server. The vulnerability stems from failing to sanitize 
'.%2e/', more commonly seen as  ../, which navigates to a parent directory. Our project uses the exploit to download and run a beach head. This beach head in turn downloads, installs, and runs the C2 implant. Data exfiltration will be added for a later milestone.

## Target Setup Instructions
Refer to `./docs/env-setup-apache2_4_49.md` for how to set up the vulnerable target VM.

## Build/Run Instructions
To run this code:
```bash
sudo python3 trigger.py
```

This script runs all stages of our pipeline, including building and hosting the compiled files as well as launching the C2 and exfil servers.  

## Threat-Model Diagram
