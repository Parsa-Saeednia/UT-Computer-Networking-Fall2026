# Assignment Specification: UT-Chat & Network Utilities (CA1)

**Course**: Computer Networks (Fall 1405–1406 / 2026)  
**Instructor**: Prof. Nasser Yazdani  
**TAs**: Sadegh Abedi, Mohammadmehdi Eghlima  

---

## 1. Overview
The goal of this assignment is to develop low-level network client and server applications using the standard POSIX Berkeley Sockets API in C/C++ without high-level networking abstractions. The assignment contains three primary parts:
1. **UDP Pinger**: UDP-based latency and packet-loss measurement system with simulated drop rates and monotonic RTT timers.
2. **TCP Chat (UT-Chat)**: Multi-room, concurrent IRC-like chat server and non-blocking client supporting UTF-8 line framing, room isolation, message sequencing, private messaging, and presence events.
3. **FTP Server & Client (MODE B)**: Out-of-band control and passive dynamic data connection architecture for flat-namespace file listing, atomic uploading (STOR), and downloading (RETR) using block mode (MODE B) with 3-byte headers.

---

## 2. Compilation and Command Line Interface

### 2.1 Build Rules
The project must include a `Makefile` or `build.sh` capable of compiling all six binaries under GCC/G++ in a Linux/WSL environment:
- `ping-server`
- `ping-client`
- `chat-server`
- `chat-client`
- `ftp-server`
- `ftp-client`

### 2.2 Execution Arguments
Ports must never be hardcoded. Servers must bind to `0.0.0.0` (or `127.0.0.1`).

| Binary | Command-Line Arguments |
| :--- | :--- |
| `ping-server` | `./ping-server --port P --loss L [--seed S]` |
| `ping-client` | `./ping-client --host H --port P [--count N] [--timeout MS] [--interval MS]` |
| `chat-server` | `./chat-server --port P` |
| `chat-client` | `./chat-client --host H --port P` |
| `ftp-server` | `./ftp-server --port P --root DIR` |
| `ftp-client` | `./ftp-client --host H --port P` |

---

## 3. Part 1: UDP Pinger

### 3.1 Datagram Format
Every packet is an ASCII datagram (max 512 bytes). An optional trailing `\n` is accepted. Fields are separated by exactly one space:
- **Client to Server**: `PING <seq> <token>`
  - `<seq>`: Non-negative integer (up to 10 decimal digits, e.g. `1` to `N`).
  - `<token>`: 1 to 64 printable ASCII characters (ASCII codes 33 to 126 inclusive).
- **Server to Client**: `PONG <seq> <token>`
  - Echoes `<seq>` and `<token>` unchanged.

### 3.2 Server Behavior
- **Loss Simulation**: For each valid `PING`, if a pseudo-random float in $[0.0, 1.0)$ is $< L$ (where `--loss L` has default `0.0`), the packet is silently dropped. Otherwise, `PONG <seq> <token>` is returned.
- **RNG Seed**: If `--seed S` is specified, `srand(S)` is initialized; otherwise system time is used.
- **Error**: If datagram is malformed (invalid token, non-numeric seq, non-ASCII, or wrong command), server immediately responds with `ERR BAD_REQUEST` without applying loss.

### 3.3 Client Behavior
- Sequentially transmits pings with `seq` from `1` to `N` (default `N = 10`).
- Records send time using a monotonic clock (`std::chrono::steady_clock`).
- Waits up to `--timeout MS` (default `1000` ms) for `PONG`:
  - If a valid `PONG` arrives with matching `seq`, calculates RTT in milliseconds with at least 3 decimal places and prints:
    ```text
    PONG seq=<seq> rtt=<ms> ms
    ```
  - If timeout expires before receiving matching reply: prints `TIMEOUT seq=<seq>`.
  - **Late Reply / Out-of-Order Handling**: If a packet arrives with `seq != current_seq` (e.g. late reply from a previous request), the client ignores it and continues waiting until timeout expires.
- Sleeps `--interval MS` (default `0` ms) between pings (no wait after ping `N`).
- At termination, prints summary statistics and exits with code 0:
  ```text
  --- ping statistics ---
  sent=<S> received=<R> loss=<loss_percent>%
  rtt min/avg/max = <min>/<avg>/<max> ms
  ```
  If no packets are received, outputs:
  ```text
  rtt min/avg/max = -/-/- ms
  ```

---

## 4. Part 2: TCP Multi-Room Chat (UT-Chat)

### 4.1 Transport Framing
- Lines are encoded in UTF-8 and terminated by `\n` (ASCII 10).
- Server also accepts `\r\n` (stripping `\r`).
- Empty lines are ignored.
- Maximum line length is 1024 bytes (excluding trailing `\n`).
- If a line exceeds 1024 bytes, the server sends `ERR 413 LINE_TOO_LONG`, discards the remainder of that line, and keeps the connection open.
- Commands are case-sensitive and uppercase.

### 4.2 Identifier Grammar
- **Nicknames (`<nick>`)**: 1 to 16 characters from `[a-zA-Z0-9_]`.
- **Rooms (`<room>`)**: Must begin with `#` followed by 1 to 31 characters from `[a-z0-9_-]` (e.g., `#cn1405`).

### 4.3 Greeting & Commands
Upon connection, server immediately sends:
```text
HELLO UT-CHAT/1.0
```

| Command | Arguments | Success Reply | Notes |
| :--- | :--- | :--- | :--- |
| `NICK` | `<nick>` | `OK NICK <nick>` | Registers username. Must be done before any other command except `PING` or `QUIT`. |
| `JOIN` | `<room>` | `OK JOIN <room>` | Joins room (creates if non-existent). Rooms never delete. Broadcasts `EVENT JOIN <room> <nick>` to other room members. |
| `PART` | `<room>` | `OK PART <room>` | Leaves room. Broadcasts `EVENT LEAVE <room> <nick>` to other room members. |
| `MSG` | `<room> <text>` | `OK MSG <room> <id>` | Sends message to room. Server broadcasts `FROM <room> <nick> <id> <text>` to all other room members. |
| `PRIVMSG` | `<nick> <text>` | `OK PRIVMSG <nick>` | Sends private message. Server forwards `PRIV <sender_nick> <text>` to recipient. |
| `LIST` | *(none)* | `OK LIST <n>`<br>`+ n × ROOM <room> <count>` | Lists all rooms sorted alphabetically by room name. |
| `WHO` | `<room>` | `OK WHO <room> <n>`<br>`+ n × USER <nick>` | Lists members in room sorted alphabetically by nick. |
| `PING` | *(none)* | `PONG` | Connection health check. |
| `QUIT` | *(none)* | `OK BYE` | Server closes connection after sending. Broadcasts `EVENT LEAVE` to all rooms the user joined. |

### 4.4 Error Precedence & Error Matrix
When parsing a line, check errors in this strict sequence:
1. `LINE_TOO_LONG` (413)
2. `UNKNOWN_COMMAND` (400)
3. `NOT_REGISTERED` (401) — if unregistered and command is not `NICK`, `PING`, or `QUIT`.
4. `BAD_SYNTAX` (400) — bad arguments, missing `#` in room name, empty text field.
5. Semantic errors:
   - `403 NOT_IN_ROOM`: For `MSG` or `PART` if sender is not in the room.
   - `404 NO_SUCH_NICK`: For `PRIVMSG` if recipient does not exist.
   - `404 NO_SUCH_ROOM`: For `WHO` if room does not exist.
   - `409 NICK_IN_USE`: On `NICK` if requested nick is already taken.
   - `409 ALREADY_REGISTERED`: On second `NICK` command.
   - `409 ALREADY_IN_ROOM`: On `JOIN` if user is already a member.

### 4.5 Robustness & Guarantees
- **Message IDs**: Each room maintains an isolated 1-based sequential integer ID counter (`1, 2, 3...`) without gaps.
- **Atomicity**: Assigning sequence IDs and placing messages in member outgoing queues must be atomic.
- **Interleaving**: Multi-line responses (`LIST`, `WHO`) must never be split by asynchronous events.
- **Disconnection**: Client abrupt drop (FIN/RST) is treated identically to `QUIT` (frees nickname, broadcasts `EVENT LEAVE`).
- **Compatibility**: Client and server must work seamlessly with `nc 127.0.0.1 <port>`.

---

## 5. Part 3: FTP Server & Client (MODE B)

### 5.1 Architecture
FTP uses out-of-band control and passive dynamic data connections:
1. **Control Connection**: Established on `--port P`. Persists for command exchange (`\r\n` or `\n`).
2. **Data Connection**: Created ephemerally via `PASV` for `LIST`, `RETR`, and `STOR`.

### 5.2 Root Directory
- Flat directory structure specified by `--root DIR`. No nested directories allowed.
- Valid filenames match `^[a-zA-Z0-9._-]{1,64}$`. Path traversal (e.g., `..`, `/`) is strictly rejected.

### 5.3 MODE B Framing
Every block transmitted over data socket contains a 3-byte header followed by payload:
```
+----------------+--------------------+------------------------+
| Descriptor(1B) | Count (2B, BigEnd) | Payload (Count Bytes)  |
+----------------+--------------------+------------------------+
```
- `Descriptor`:
  - `0x00`: Normal data block.
  - `0x40`: EOF block (indicates end of transmission).
- `Count`: 16-bit unsigned integer (big-endian) denoting byte length of payload in this block.
- **Sender rules**:
  - Regular data blocks contain exactly 1024 bytes (`0x00 0x04 0x00` + 1024 bytes).
  - Last block has descriptor `0x40` with 1 to 1024 remaining bytes.
  - For empty files (0 bytes), sender transmits a single empty EOF block: `40 00 00`.
  - Sender closes the data connection immediately after sending the EOF block.
- **Receiver rules**:
  - Accepts blocks of any size $0$ to $65535$ bytes until EOF block is encountered.

### 5.4 Control Commands & Responses
Greeting on control connect:
```text
220 UT-FTP ready
```

| Command | Parameter | Response | Meaning |
| :--- | :--- | :--- | :--- |
| `USER` | `<name>` | `331 Password required` | Accepts any username. |
| `PASS` | `<password>` | `230 Logged in` | Accepts any password after `USER`. |
| `MODE` | `B` | `200 OK` | Only `B` allowed. Other modes yield `504`. |
| `PASV` | *(none)* | `227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)` | Opens listening data socket on port `p1*256 + p2`. |
| `LIST` | *(none)* | `150 Opening data connection`<br>`226 Transfer complete` | Sends list of `<filename> <size>\n` over data connection. |
| `RETR` | `<file>` | `150 Opening data connection`<br>`226 Transfer complete` | Streams file data to client in MODE B blocks. |
| `STOR` | `<file>` | `150 Opening data connection`<br>`226 Transfer complete` | Receives file data from client in MODE B blocks atomically. |
| `QUIT` | *(none)* | `221 Goodbye` | Server closes control connection. |

### 5.5 Error Precedence for Transfer Commands
Evaluate errors in this exact order:
1. `530 NOT_LOGGED_IN`: If not logged in and command is not `USER`, `PASS`, or `QUIT`.
2. `501 SYNTAX_ERROR_PARAMS`: Missing argument.
3. `553 NAME_NOT_ALLOWED`: Invalid filename (contains space, `/`, `..`, or length > 64).
4. `503 BAD_SEQUENCE`: Transfer attempted without prior `MODE B`, or `PASS` without `USER`.
5. `425 CANT_OPEN_DATA_CONNECTION`: Transfer issued without prior unused `PASV`.
6. `550 FILE_NOT_FOUND`: For `RETR`, if file does not exist in root directory.
7. `426 CONNECTION_CLOSED`: Premature data close or invalid block descriptor before receiving EOF block.

### 5.6 Atomic STOR
When receiving `STOR <file>`:
- Data is written to an isolated temporary file (e.g. `.upload_tmp_XXXXXX`).
- Only after receiving a valid `0x40` EOF block is the file atomically renamed to `<file>`.
- If transfer aborts or encounters an error, the temporary file is unlinked; any existing file remains unmodified.

