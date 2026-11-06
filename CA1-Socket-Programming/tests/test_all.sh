#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

echo "=================================================="
echo " Starting CA1 Automated Verification Suite "
echo "=================================================="

echo ">>> [1/4] Compiling all targets with Makefile..."
make clean
make all

test_passed=0
test_total=0

pass() {
    test_passed=$((test_passed + 1))
    test_total=$((test_total + 1))
    echo "  [PASS] $1"
}

fail() {
    test_total=$((test_total + 1))
    echo "  [FAIL] $1"
    exit 1
}

echo ">>> [2/4] Testing UDP Pinger..."
./ping-server --port 19001 --loss 0.0 &
SERVER_PID=$!
sleep 0.5

PING_OUT=$(./ping-client --host 127.0.0.1 --port 19001 --count 5 --timeout 500 --interval 20)
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true

if echo "$PING_OUT" | grep -q "sent=5 received=5 loss=0.0%"; then
    pass "UDP Pinger 0% loss count=5"
else
    echo "$PING_OUT"
    fail "UDP Pinger failed to receive all packets"
fi

./ping-server --port 19002 --loss 0.0 &
SERVER_PID=$!
sleep 0.5
BAD_RESP=$(python3 -c "
import socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s.settimeout(1.0)
s.sendto(b'INVALID_COMMAND\n', ('127.0.0.1', 19002))
data, _ = s.recvfrom(512)
print(data.decode().strip())
")
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true

if [ "$BAD_RESP" = "ERR BAD_REQUEST" ]; then
    pass "UDP Server returns ERR BAD_REQUEST on invalid packet"
else
    fail "UDP Server returned '$BAD_RESP' instead of ERR BAD_REQUEST"
fi

echo ">>> [3/4] Testing TCP Chat..."
./chat-server --port 18081 &
CHAT_PID=$!
sleep 0.5

CHAT_TEST_RESULT=$(python3 -c "
import socket, time

s1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s1.connect(('127.0.0.1', 18081))
h1 = s1.recv(1024).decode()
assert 'HELLO UT-CHAT/1.0' in h1, f'Unexpected greeting: {h1}'

s1.sendall(b'MSG #cn hello\n')
resp = s1.recv(1024).decode()
assert 'ERR 401 NOT_REGISTERED' in resp, f'Expected 401, got {resp}'

s1.sendall(b'NICK alice\n')
assert 'OK NICK alice' in s1.recv(1024).decode()

s1.sendall(b'JOIN #cn1405\n')
assert 'OK JOIN #cn1405' in s1.recv(1024).decode()

s2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s2.connect(('127.0.0.1', 18081))
assert 'HELLO UT-CHAT/1.0' in s2.recv(1024).decode()
s2.sendall(b'NICK bob\n')
assert 'OK NICK bob' in s2.recv(1024).decode()
s2.sendall(b'JOIN #cn1405\n')
assert 'OK JOIN #cn1405' in s2.recv(1024).decode()

time.sleep(0.1)
alice_event = s1.recv(1024).decode()
assert 'EVENT JOIN #cn1405 bob' in alice_event, f'Expected join event, got {alice_event}'

s1.sendall(b'MSG #cn1405 salam bob!\n')
alice_msg_ack = s1.recv(1024).decode()
assert 'OK MSG #cn1405 1' in alice_msg_ack, f'Expected OK MSG 1, got {alice_msg_ack}'

time.sleep(0.1)
bob_msg = s2.recv(1024).decode()
assert 'FROM #cn1405 alice 1 salam bob!' in bob_msg, f'Expected FROM msg, got {bob_msg}'

s1.sendall(b'WHO #cn1405\n')
who_resp = s1.recv(1024).decode()
assert 'OK WHO #cn1405 2' in who_resp
assert 'USER alice' in who_resp
assert 'USER bob' in who_resp

oversized = b'PING ' + b'A' * 1100 + b'\n'
s1.sendall(oversized)
too_long_resp = s1.recv(1024).decode()
assert 'ERR 413 LINE_TOO_LONG' in too_long_resp, f'Expected 413, got {too_long_resp}'

s1.sendall(b'QUIT\n')
assert 'OK BYE' in s1.recv(1024).decode()

time.sleep(0.1)
bob_leave = s2.recv(1024).decode()
assert 'EVENT LEAVE #cn1405 alice' in bob_leave, f'Expected leave event, got {bob_leave}'

s2.close()
s1.close()
print('SUCCESS')
")

kill $CHAT_PID 2>/dev/null || true
wait $CHAT_PID 2>/dev/null || true

if [ "$CHAT_TEST_RESULT" = "SUCCESS" ]; then
    pass "TCP Chat protocol, multi-client events & error hierarchy"
else
    fail "TCP Chat test failed: $CHAT_TEST_RESULT"
fi

echo ">>> [4/4] Testing FTP Server & Client..."
mkdir -p test_ftp_root client_downloads
echo "Hello from UT-FTP test suite!" > test_ftp_root/greeting.txt

./ftp-server --port 12122 --root ./test_ftp_root &
FTP_PID=$!
sleep 0.5

FTP_TEST_RESULT=$(python3 -c "
import socket, struct

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('127.0.0.1', 12122))
assert '220 UT-FTP ready' in s.recv(1024).decode()

s.sendall(b'USER alice\r\n')
assert '331' in s.recv(1024).decode()
s.sendall(b'PASS secret\r\n')
assert '230' in s.recv(1024).decode()

s.sendall(b'MODE B\r\n')
assert '200' in s.recv(1024).decode()

s.sendall(b'PASV\r\n')
pasv_resp = s.recv(1024).decode()
assert '227' in pasv_resp
lp = pasv_resp.find('(')
rp = pasv_resp.find(')')
parts = [int(x) for x in pasv_resp[lp+1:rp].split(',')]
data_port = parts[4]*256 + parts[5]

dsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
dsock.connect(('127.0.0.1', data_port))
s.sendall(b'LIST\r\n')
assert '150' in s.recv(1024).decode()

listing = b''
while True:
    hdr = dsock.recv(3)
    if not hdr: break
    desc, count = struct.unpack('!BH', hdr)
    if count > 0:
        listing += dsock.recv(count)
    if desc == 0x40:
        break
dsock.close()
assert '226' in s.recv(1024).decode()
assert b'greeting.txt' in listing

s.sendall(b'PASV\r\n')
pasv_resp = s.recv(1024).decode()
parts = [int(x) for x in pasv_resp[pasv_resp.find('(')+1:pasv_resp.find(')')].split(',')]
data_port = parts[4]*256 + parts[5]

dsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
dsock.connect(('127.0.0.1', data_port))
s.sendall(b'RETR greeting.txt\r\n')
assert '150' in s.recv(1024).decode()

retrieved = b''
while True:
    hdr = dsock.recv(3)
    if not hdr: break
    desc, count = struct.unpack('!BH', hdr)
    if count > 0:
        retrieved += dsock.recv(count)
    if desc == 0x40:
        break
dsock.close()
assert '226' in s.recv(1024).decode()
assert retrieved == b'Hello from UT-FTP test suite!\n'

s.sendall(b'PASV\r\n')
pasv_resp = s.recv(1024).decode()
parts = [int(x) for x in pasv_resp[pasv_resp.find('(')+1:pasv_resp.find(')')].split(',')]
data_port = parts[4]*256 + parts[5]

dsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
dsock.connect(('127.0.0.1', data_port))
s.sendall(b'STOR uploaded.txt\r\n')
assert '150' in s.recv(1024).decode()

upload_content = b'Content uploaded atomically via MODE B!'
hdr = struct.pack('!BH', 0x40, len(upload_content))
dsock.sendall(hdr + upload_content)
dsock.close()

assert '226' in s.recv(1024).decode()

s.sendall(b'QUIT\r\n')
assert '221' in s.recv(1024).decode()
s.close()
print('SUCCESS')
")

kill $FTP_PID 2>/dev/null || true
wait $FTP_PID 2>/dev/null || true

if [ -f "test_ftp_root/uploaded.txt" ]; then
    UP_CONTENT=$(cat test_ftp_root/uploaded.txt)
    if [ "$UP_CONTENT" = "Content uploaded atomically via MODE B!" ]; then
        pass "FTP Mode B transfer, Passive Mode & Atomic STOR verified"
    else
        fail "Uploaded content mismatch"
    fi
else
    fail "Uploaded file not found in test_ftp_root"
fi

rm -rf test_ftp_root client_downloads tmp

echo "=================================================="
echo " All $test_passed / $test_total Tests Passed Successfully! "
echo "=================================================="

