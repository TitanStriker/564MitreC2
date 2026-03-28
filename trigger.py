import subprocess
import sys
import time

# 1. The script that stays open
server_script = 'c2server.py'

# 2. The scripts that run and then exit naturally
task_scripts = ['exfilserver.py', 'trigger.py']

processes = []

def launch_script(name, wait=False):
    """Launches a script and returns the process object."""
    try:
        proc = subprocess.Popen([sys.executable, name])
        print(f"Launched {name} (PID: {proc.pid})")
        return proc
    except FileNotFoundError:
        print(f"Error: Could not find '{name}'")
        return None

# Start the one-off tasks first
for script in task_scripts:
    launch_script(script)

# Start the main server
server_proc = launch_script(server_script)

print("\n--- Tasks are running and the server is live ---")
print("Press Ctrl+C to shut down the main server.")

try:
    # Keep the script alive as long as the server is running
    if server_proc:
        server_proc.wait() 
except KeyboardInterrupt:
    print("\nShutting down...")
    if server_proc:
        server_proc.terminate()
    print("Clean exit.")