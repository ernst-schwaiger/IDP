# acc

Adaptive cruise control project

Prerequisites:
```bash
sudo apt update && sudo apt install -y \
    g++ \
    cmake \
    make \
    gcovr \
    pkg-config \
    libbluetooth-dev \
    bluez \
    libglib2.0-dev \
    qt6-base-dev \
    qt6-tools-dev \
    qtcreator \
    qt6-wayland-dev
```

running qt creator on WSL: Set environment variable to tell qt to use X11 instead of
(default) wayland before execution `QT_QPA_PLATFORM=xcb qtcreator`.



for building the documentation:
```bash
sudo apt install -y texlive latexmk texlive-latex-extra
```

## Build

In order for cmake to find libtommath in the acc project, check it out and build it in a local folder

```bash
git clone https://github.com/libtom/libtommath.git && \
cd libtommath && \
mkdir -p build && \
cd build && \
cmake .. && \
make -sj
```

Build the `node1`, `node2` release binaries as `acc/build/node1/node1` and `acc/build/node2/node2`:
```bash
cd acc && \
mkdir -p release && \
cd release && \
cmake -DCMAKE_BUILD_TYPE=Release .. && \
make -sj
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
cd acc && \
mkdir debug && \
cd debug && \
cmake -DCMAKE_BUILD_TYPE=Debug .. && \
make -j
```

### Execute Unit Tests
Either `ctest --output-on-failure` or run a test binary, e.g. `./libraries/bluetooth/bluetooth_test`.
### Generate Coverage Report
```
cmake --build . --target coverage
```
in a debug/release folder generates a `./libraries/CryptoComm/coverage.html` indicating covered/not covered parts of the code.

## Develop on remote node using local VS Code

VS Code supports to connect to a remote node, and to execute as if VS code was running on that remote machine, including compilation, debugging, code editing,...

- Preconditions: SSH server executing on the remote host, using ssh keys for authentication
- Install "Remote Development" extension pack from the VS Code extensions menu.
- Open Command Pallette via Ctrl-Shift-P
- Select "Remote-SSH: Connect to Host...", add `user@node` in the popup window, `user` being your account on the remote machine and `node` being the name or IP of the remote machine.
- A new window opens up, setting up VS Code for the remote node, this takes a few minutes
- Select "Linux" as platform
- You will be asked if you trust the remote node
- After the installation is done, open the project folder (VS Code will display the folders on the remote machine).
- When compiling, debugging, it may be necessary to (re-) install some of the extensions, C++, CMake, ... "Install in SSH: <node>" to have it available.
- As a shortcut, switch to the "Remote Explorer" View in VS Code, select "Remotes (Tunnels/SSH)" in the top combo box, use the "+" icon to add a new remote host to connect to, and add the ssh command, e.g. `ssh uer@node`. The configuration is stored and can be actrivated in the future from the Remotes View.

See also:
- https://code.visualstudio.com/docs/remote/ssh
- https://learn.microsoft.com/en-us/training/modules/develop-on-remote-machine/

## TODOs

- detailed concept for acc
- check: is encryption needed for bluetooth comm, or is MAC with counter sufficient?
- fix coverage
- implement async send/receive in BT sockets.
- implement async reading of proximity sensors

## Open Topics
- Intro to project structure: CMake, LaTeX, VS Code plugins. Howto build, howto debug
- Which OSes shall we use? Debuggability, real-time properties, boot time, ... (ask Mattias?)
- MISRA allows exceptions in C++ code for error handling. OK in our projects as well? (Julia Teissl)
- Encrypted messages/messages with MAC? Which Crypto Library? MAC, LibTomCrypt
- Define Message Layout: node1->node2: (Converted) Sensor readings, message counter, MAC (Ernst)
- Define Message Layout: node2->node1: Request ID, parameters, message counter, MAC (Ernst)
- Discuss: Usage of 3 inch display with touch function: Has anyone used such a device yet?
- Fault injector modules to test our software when it detects failures
- Software design: Decouple "business logic" of our app from concrete HW to make it testable.
- Add Unit Tests, see Catch2 Tests, have a look at the coverage report.
- Who takes which tasks?
-- Stefan: Node1, proximity devices
-- Lorenzo: Touch Display
-- Ernst: Communication/Crypto :-).