rule C2_Beachhead {
    meta:
        description = "Detects the initial beachhead/stage 1 loader"
        author = "TheRealWorld"
        date = "2026-05-06"
        version = "1.0"

    strings:
        $s1 = "[PRIVESC CHECK]"
        $s2 = "[+] Successfully gained sudo privileges!"
        $s3 = "[!] Failed to download certificate for exfil"
        $s4 = "[*] Stage 1: Checking privilege escalation conditions..."
        $s5 = "[*] Stage 2: Attempting privilege escalation..."
        $s6 = "[*] Stage 3: Downloading implant..."
        $s7 = "/tmp/systemd-private-update"
        $p1 = "id -nu 2>/dev/null"
        $p2 = "wget -q '%s' -O '%s' >/dev/null 2>&1"

    condition:
        uint16(0) == 0x457f or uint16(0) == 0x5a4d or any of ($s*) or all of ($p*)
}

rule C2_Implant {
    meta:
        description = "Detects the primary C2 implant"
        author = "TheRealWorld"
        date = "2026-05-06"

    strings:
        $s1 = "HELLO from implant"
        $s2 = "=== FULL RECON REPORT ==="
        $s3 = "=== FULL LOCATION REPORT ==="
        $s4 = "=== CONTAINER ESCAPE REPORT ==="
        $s5 = "SELF_DESTRUCT: Initiating cleanup and self-deletion"
        $s6 = "ESCAPE_OK"
        $s7 = "ESCAPE_FAIL"
        $s8 = "NOT_DOCKER"
        $s9 = "SELF_DESTRUCT_INIT"
        
        // Stealthy paths used in LVM escape
        $path1 = "/tmp/.font-unix-s"
        $path2 = "/tmp/.font-unix-d"
        $path3 = "/var/tmp/.system-cache"
        
        // Commands used in Docker escape
        $c1 = "vgscan >"
        $c2 = "vgchange -ay"
        $c3 = "vgmknodes"
        $c4 = "lvdisplay -c"

    condition:
        4 of ($s*) or (all of ($path*) and any of ($c*))
}

rule C2_Shared_Logic {
    meta:
        description = "Detects shared logic and helper strings in C2 components"
        author = "TheRealWorld"

    strings:
        // Stealthy cron check strings
        $cron1 = "cron.service"
        $cron2 = "/run/crond.pid"
        $cron3 = "/var/run/crond.pid"
        $cron4 = "/proc/%d/comm"
        
        // Recon strings
        $rec1 = "Security Products: "
        $rec2 = "--- NETWORK INTERFACES ---"
        $rec3 = "--- SSH Authorized Keys ---"
        
        // Command types
        $cmd1 = "HELO" fullword
        $cmd2 = "RECON" fullword
        $cmd3 = "IP_REPORT" fullword
        $cmd4 = "ESCAPE" fullword
        $cmd5 = "SELF_DESTRUCT" fullword

    condition:
        (3 of ($cron*) and any of ($rec*)) or (all of ($cmd*))
}

rule C2_Python_Server {
    meta:
        description = "Detects the Python C2 Server"
        author = "TheRealWorld"

    strings:
        $t1 = "types = ['HELO', 'EXIT', 'READ', 'RITE', 'CMD', 'ERR', 'RECON', 'IP_REPORT', 'ESCAPE', 'SELF_DESTRUCT', 'GET']"
        $t2 = "def parseAndSendInput():"
        $t3 = "def receiveMessage(c: ssl.SSLSocket):"
        $t4 = "Listening on {host}:{port} (TLS)"
        $t5 = "openssl req -x509 -newkey rsa:4096"

    condition:
        2 of ($t*)
}
