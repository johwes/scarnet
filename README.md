# scarnet

A small text-protocol message broker written in C, used as a security analysis training target.

## Protocol

scarnet speaks a line-oriented text protocol (newline-terminated commands):

```
AUTH <user> <pass>               — authenticate the session
SET  <key> <value>               — store a key-value pair
GET  <key>                       — retrieve a value
DEL  <key>                       — delete a key-value pair
STATS                            — server statistics
FRAG <id> <offset> <total> <len> — send one fragment of a multi-part message
QUIT                             — end session
```

Sessions start unauthenticated. All commands except `AUTH` require a prior
successful `AUTH`.

### Fragmented messages (FRAG)

Large payloads can be split across multiple `FRAG` commands. The receiver
reassembles fragments into a contiguous buffer identified by `<id>` (0–15).

```
FRAG 0 0   128 64   → fragment 0 of stream 0: bytes 0–63   of a 128-byte message
FRAG 0 64  128 64   → fragment 1 of stream 0: bytes 64–127
```

When `written >= total` the stream is complete.

## Layout

```
include/scarnet.h   — shared types and function declarations
src/parse.c         — command tokeniser
src/handler.c       — command dispatcher (SET, GET, DEL, STATS, FRAG)
src/session.c       — session state and fragment reassembly
src/util.c          — logging and string helpers
fuzz_target.c       — libFuzzer entry point
build.sh            — OSS-Fuzz compatible build script
Makefile            — local build with AddressSanitizer
```

## Building and running

```bash
# Build the server (default)
make

# Run on the default port (4242)
./scarnet

# Run on a custom port
./scarnet 9000
```

Connect with netcat and interact with the protocol:

```
$ nc localhost 4242
scarnet 0.1.0 ready
AUTH user scarnet123
OK authenticated
SET greeting hello
OK
GET greeting
VALUE hello
STATS
ENTRIES 1
OPS 3
AVG_VAL_LEN 5
QUIT
OK bye
```

## Fuzzing

```bash
# Build fuzz target with AddressSanitizer (recommended)
make fuzz-asan

# Run the fuzzer
./scarnet_fuzz -max_len=4096 corpus/
```

Or with the OSS-Fuzz build script:

```bash
OUT=./build bash build.sh
```

## Using with SCAR

Point the SCAR pipeline at this repository:

```bash
tkn pipeline start scar \
  --param repo-url=https://github.com/johwes/scarnet \
  --workspace name=shared-data,claimName=scar-pvc \
  --showlog
```
