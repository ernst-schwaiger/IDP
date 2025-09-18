# acc

Adaptive cruise control project

Prerequisites:
```bash
sudo apt install -Y g++ cmake make gcovr pkg-config bluez libglib2.0-dev
```

If a libdbus library is missing, add
```bash
sudo apt install -Y libdbus-1-dev
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

## Run the Application

### Node1

- run `bluetoothctl show`, write down the MAC address
- run `bluetoothctl discoverable yes`
- run the `node1` executable 

### Node2

- run `node2 <MAC_ADDRESS>`, where `<MAC_ADDRESS>` is the written down MAC address of node 1

## Unit Test and Coverage
Build the binaries in `acc/debug`:
```
cd acc
mkdir debug
cd debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j
```

### Execute Unit Tests
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