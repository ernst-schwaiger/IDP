# acc

Adaptive cruise control project

Prerequisites:
```
sudo apt install -Y g++ cmake make gcovr
```
## Build

Build the `node1`, `node2` release binaries as `acc/build/node1/node1` and `acc/build/node2/node2`:
```
cd acc
mkdir release
cd release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```

## Test and Coverage
Build the binaries in `acc/debug`:
```
cd acc
mkdir debug
cd debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j
```

### Execute Tests
Either `ctest --output-on-failure` or run a test binary, e.g. `./libraries/bluetooth/bluetooth_test`.
### Generate Coverage Report
```
make coverage
```
generates a `Peer/debug/coverage.html` indicating covered/not covered parts of the code.

## TODOs

- Add proper bluetooth API to common library
- detailed concept for acc
- check: is encryption needed for bluetooth comm, or is MAC with counter sufficient?
- integrate crypto library
- fix coverage
- implement async send/receive in BT sockets.