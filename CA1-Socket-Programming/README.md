# Computer Networks (Fall 2026 / 1405–1406)
## Computer Assignment 1: Socket Programming (UT-Chat)

**Department of Electrical and Computer Engineering**  
**University of Tehran**  

- **Instructor**: Prof. Nasser Yazdani  
- **Teaching Assistants**: Sadegh Abedi, Mohammadmehdi Eghlima  

---

## 1. System Architecture Overview

This project implements three fundamental network services from the ground up using POSIX Berkeley Sockets in Modern C++ (C++17) without external high-level networking libraries:

### Part 1: UDP Pinger (`ping-server`, `ping-client`)
- **Transport**: UDP (`SOCK_DGRAM`).
- **Functionality**: Latency, jitter, and packet loss measurement.
- **Server Mechanics**: Simulates network unreliability via configurable packet drop probability (`--loss L`) with reproducible pseudo-random sequences (`--seed S`). Accurately distinguishes malformed datagrams from dropped packets by immediately replying with `ERR BAD_REQUEST` without applying loss.
- **Client Mechanics**: Uses a high-resolution monotonic clock (`std::chrono::steady_clock`) to measure Round-Trip Time (RTT) down to millisecond precision. Out-of-order or late responses corresponding to earlier sequence numbers are filtered without aborting the pending wait window.

### Part 2: TCP Multi-Room Chat (`chat-server`, `chat-client`)
- **Transport**: TCP (`SOCK_STREAM`).
- **Concurrency**: Event-driven single-threaded I/O multiplexing using POSIX `poll()`.
- **Framing**: UTF-8 text streams delimited by `\n` (with transparent `\r` removal). Enforces a 1024-byte line limit with `ERR 413 LINE_TOO_LONG`.
- **Room & Sequence Isolation**: Each room maintains an isolated monotonic 1-based integer message ID counter (`1, 2, ...`). Broadcasts to room members are atomic.
- **Error Hierarchy**: Implements strict evaluation precedence:
  `LINE_TOO_LONG (413)` $\rightarrow$ `UNKNOWN_COMMAND (400)` $\rightarrow$ `NOT_REGISTERED (401)` $\rightarrow$ `BAD_SYNTAX (400)` $\rightarrow$ Semantic checks (`403 NOT_IN_ROOM`, `404 NO_SUCH_NICK/ROOM`, `409 NICK_IN_USE/ALREADY_REGISTERED/ALREADY_IN_ROOM`).
- **Presence & Lifecycle**: Asynchronous broadcast of join and departure events (`EVENT JOIN`, `EVENT LEAVE`). Abrupt connection termination (FIN/RST) is handled gracefully without crashing or resource leaks.

### Part 3: FTP Server & Client with Mode B (`ftp-server`, `ftp-client`)
- **Transport**: Dual-connection architecture: persistent out-of-band control connection and dynamic passive data connections (`PASV`).
- **Namespace Rules**: Flat file storage in `--root DIR` with strict character set validation (`[a-zA-Z0-9._-]{1,64}`), preventing directory traversal attacks (`/`, `..`).
- **Mode B Block Framing**: Binary transmission structured into 3-byte headers:
  - Byte 0: Descriptor (`0x00` for normal data block, `0x40` for EOF block).
  - Bytes 1–2: 16-bit big-endian length count ($N$).
  - Bytes $3 \dots (N+2)$: Block payload.
- **Atomic STOR**: Uploaded data streams into isolated temporary staging files (`.upload_tmp_*`). The file is committed to its destination name via atomic `rename()` only after verifying the final EOF block (`0x40`). Aborted or truncated connections trigger immediate unlinking of staging files, preserving disk integrity.

---

## 2. Directory Structure

```text
CA1-Socket-Programming/
├── docs/
│   └── SPECIFICATION.md          # Exhaustive protocol & requirements specification
├── src/
│   ├── common/                   # Shared infrastructure & utilities
│   │   ├── config.hpp            # Protocol constants & defaults
│   │   ├── exception.hpp         # Typed domain exception hierarchy
│   │   ├── utils.hpp             # Helper function declarations
│   │   └── utils.cpp             # Clean utility implementations
│   ├── pinger/                   # Part 1: UDP Pinger
│   │   ├── ping_server.hpp / cpp # Server implementation & loss simulation
│   │   ├── ping_client.hpp / cpp # Client implementation & monotonic timers
│   │   ├── server_main.cpp       # CLI entrypoint for ping-server
│   │   └── client_main.cpp       # CLI entrypoint for ping-client
│   ├── chat/                     # Part 2: TCP Multi-room Chat
│   │   ├── protocol.hpp / cpp    # Protocol parsing & response formatters
│   │   ├── room_manager.hpp/cpp  # Room membership & message sequencing
│   │   ├── chat_server.hpp / cpp # Event loop (poll) & connection handling
│   │   ├── chat_client.hpp / cpp # Stdin and socket multiplexer
│   │   ├── server_main.cpp       # CLI entrypoint for chat-server
│   │   └── client_main.cpp       # CLI entrypoint for chat-client
│   └── ftp/                      # Part 3: FTP with Mode B
│       ├── mode_b.hpp / cpp      # Mode B block framing & streaming
│       ├── ftp_session.hpp / cpp # Client session state machine
│       ├── ftp_server.hpp / cpp  # Control server listener & session runner
│       ├── ftp_client.hpp / cpp  # Client CLI & passive transfer handler
│       ├── server_main.cpp       # CLI entrypoint for ftp-server
│       └── client_main.cpp       # CLI entrypoint for ftp-client
├── tests/
│   └── test_all.sh               # Automated end-to-end integration test suite
├── Makefile                      # Standard build automation with build/ output
├── build.sh                      # Shell build script wrapper
└── README.md                     # Academic project documentation
```

---

## 3. Compilation & Build

### Requirements
- **OS**: Linux (Ubuntu 20.04+ or WSL2 recommended)
- **Compiler**: GCC/G++ $\ge 9.0$ with C++17 support
- **Build Tool**: GNU Make

### Build Commands
Compile all six targets:
```bash
make all
```

Or compile individual subsystems:
```bash
make pinger   # Compiles ping-server and ping-client
make chat     # Compiles chat-server and chat-client
make ftp      # Compiles ftp-server and ftp-client
```

All compiled object files are isolated in the `build/` directory, and executable binaries are placed directly in the project root:
- `./ping-server`
- `./ping-client`
- `./chat-server`
- `./chat-client`
- `./ftp-server`
- `./ftp-client`

To clean all compiled objects and executables:
```bash
make clean
```

Alternatively, run the build script wrapper:
```bash
chmod +x build.sh
./build.sh
```

---

## 4. Execution & CLI Reference

### 4.1 UDP Pinger
```bash
# Start UDP Ping Server
./ping-server --port 9000 --loss 0.2 --seed 42

# Run UDP Ping Client
./ping-client --host 127.0.0.1 --port 9000 --count 10 --timeout 1000 --interval 100
```

### 4.2 TCP Multi-Room Chat
```bash
# Start Chat Server
./chat-server --port 8080

# Connect via Chat Client
./chat-client --host 127.0.0.1 --port 8080

# Verify using Netcat
nc 127.0.0.1 8080
```

### 4.3 FTP Server & Client
```bash
# Prepare root directory and start FTP Server
mkdir -p ftp_root
./ftp-server --port 2121 --root ./ftp_root

# Connect via FTP Client
./ftp-client --host 127.0.0.1 --port 2121
```

---

## 5. Verification & Testing

An automated regression test suite is included under `tests/test_all.sh`:
```bash
chmod +x tests/test_all.sh
./tests/test_all.sh
```

The test harness validates:
1. **UDP Pinger**: Ping-pong exchange, RTT calculation, drop simulation with seeds, and `ERR BAD_REQUEST` rejection of invalid payloads.
2. **TCP Chat**: Handshake greeting, registration error handling (`401`), room isolation, event broadcasting (`EVENT JOIN`/`EVENT LEAVE`), sequential message indexing, `WHO` sorting, and 1024-byte line cutoff (`413`).
3. **FTP**: Out-of-band control flow, dynamic `PASV` allocation, directory listings, atomic upload of binary files, and SHA256 checksum equivalence upon retrieval.

---

## 6. Tools, Libraries & AI Disclosure

### Tools & Libraries Used
- **Language**: C++17 (`-std=c++17 -Wall -Wextra -pedantic -O2 -pthread`)
- **APIs**: Standard POSIX Sockets (`<sys/socket.h>`, `<netinet/in.h>`, `<arpa/inet.h>`), I/O Multiplexing (`<poll.h>`), POSIX File Operations (`<dirent.h>`, `<sys/stat.h>`, `<unistd.h>`)
- **Containers & Time**: C++ Standard Library (`<vector>`, `<unordered_map>`, `<set>`, `<string>`, `<chrono>`)
- **Testing & Verification**: Bash and Python 3 standard library sockets (`socket`, `struct`) for integration tests; Netcat (`nc`) for raw connection tests.

### Artificial Intelligence Usage Disclosure
In accordance with course guidelines:
- An AI pair-programming assistant was used for scaffolding initial file templates, translating specification documentation into structured English, and generating end-to-end integration test harnesses.
- All network logic, protocol state machines, block serialization, error precedence implementations, and socket descriptor lifecycles were reviewed, structured, and validated by the team.

---

## 7. Academic References & Citations

1. **Beej's Guide to Network Programming**: Using Internet Sockets, Brian "Beej Jorgensen" Hall.
2. **RFC 959**: *File Transfer Protocol (FTP)*, J. Postel, J. Reynolds, 1985.
3. **RFC 2812**: *Internet Relay Chat: Client Protocol*, C. Kalt, 2000.
4. **IEEE Std 1003.1-2017**: *Standard for Information Technology—Portable Operating System Interface (POSIX)*, The Open Group / IEEE.
5. **Computer Networking: A Top-Down Approach (8th Edition)**, James F. Kurose, Keith W. Ross, Pearson.
