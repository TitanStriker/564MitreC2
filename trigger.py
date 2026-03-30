import subprocess
import sys
import time

# CONFIGURATION
HTTP_SERVER_PORT = 443
DOCKER_IMAGE_NAME = 'my-c2-server'
DOCKER_SERVER_PORT = 80
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

# Build and run Docker container
print("Building Docker image...")
try:
    subprocess.run(['docker', 'build', '-t', DOCKER_IMAGE_NAME, '.'], check=True)
    print("Docker image built successfully")
except subprocess.CalledProcessError as e:
    print(f"Error building Docker image: {e}")
    http_server.terminate()
    sys.exit(1)

print(f"Starting Docker container on port {DOCKER_SERVER_PORT}...")
try:
    docker_container = subprocess.Popen(['docker', 'run', '--rm', '-p', f'{DOCKER_SERVER_PORT}:{DOCKER_SERVER_PORT}', DOCKER_IMAGE_NAME])
    print(f"Docker container started (PID: {docker_container.pid})")
except Exception as e:
    print(f"Error starting Docker container: {e}")
    http_server.terminate()
    sys.exit(1)

time.sleep(1)

# Run first stage script
print(f"Running {FIRST_STAGE_SCRIPT}...")
try:
    first_stage = subprocess.Popen([sys.executable, FIRST_STAGE_SCRIPT])
    print(f"{FIRST_STAGE_SCRIPT} started (PID: {first_stage.pid})")
except FileNotFoundError:
    print(f"Error: Could not find '{FIRST_STAGE_SCRIPT}'")
    http_server.terminate()
    docker_container.terminate()
    sys.exit(1)

print("\n--- Services running ---")
print(f"  • HTTP server (scripts): port {HTTP_SERVER_PORT}")
print(f"  • server.py (Docker): port {DOCKER_SERVER_PORT}")
print(f"  • {FIRST_STAGE_SCRIPT}: running locally")
print("Press Ctrl+C to shut down.")

try:
    # Wait for first_stage.py to complete
    first_stage.wait()
    print(f"{FIRST_STAGE_SCRIPT} completed.")
    
    # Keep the services running
    print("Services are still running. Press Ctrl+C to shut down.")
    docker_container.wait()
except KeyboardInterrupt:
    print("\nShutting down...")
    http_server.terminate()
    docker_container.terminate()
    print("Clean exit.")