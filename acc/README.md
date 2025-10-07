# acc

Adaptive cruise control project; tested on Raspberry Pi 4B, 5B nodes running Raspbian OS 13/Trixie.

Install Prerequisites:
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

Running qt creator on WSL for tesing GUI on host: Set environment variable to tell qt to use X11 instead of (default) wayland before execution `QT_QPA_PLATFORM=xcb qtcreator`.

For building the design document:
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


### Troubleshooting Bluetooth Device

Depending on the used operating system, the Bluetooth device may be powered on or off, or may even be blocked from powering on via software. Whether or not an upfront pairing is required also depends on the installed operating system.

#### Unblock Bluetooth Device from Power On

In a shell, run `rfkill list`. If the output indicates "Soft blocked: yes" for the Bluetooth device, it can be unblocked via `sudo rfkill unblock bluetooth`. (Required in Raspbian OS 13 "Trixie").

#### Software Power-On Bluetooth Device

A powered-off Bluetooth device can be detected via
```bash
bluetoothctl show | grep -i "power"
Powered: no
PowerState: off
SupportedIncludes: tx-power
```
Its power can be turned on by 
```bash
bluetoothctl power on
...
bluetoothctl show | grep -i "power"
Powered: yes
PowerState: on
SupportedIncludes: tx-power
```

#### Upfront Pairing

Raspbian-based OSes (at least: Trixie and Bookworm) require an upfront pairing of the client (Node 2) towards the server (Node 1). On current Ubuntu based OSes, no upfront pairing is required.

Pairing can be done by running `bluetoothctl` on both nodes in parallel:
- Start `bluetoothctl` on Node 1
- Start `bluetoothctl` on Node 2
- On both Nodes, do
    - power on
    - agent on
- On Node 1 do
    - discoverable yes
- On Node 2 do
    - pair XX:XX:XX:XX:XX:XX (Bluetooth MAC Address of Node 1)
    - trust XX:XX:XX:XX:XX:XX (Bluetooth MAC Address of Node 1)
- On Node 1, a prompt shows up whether the MAC of Node 1 can be trusted -> yes

Both bluetooth devices are now paired.

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
- As a shortcut, switch to the "Remote Explorer" View in VS Code, select "Remotes (Tunnels/SSH)" in the top combo box, use the "+" icon to add a new remote host to connect to, and add the ssh command, e.g. `ssh user@node`. The configuration is stored and can be actrivated in the future from the Remotes View.

See also:
- https://code.visualstudio.com/docs/remote/ssh
- https://learn.microsoft.com/en-us/training/modules/develop-on-remote-machine/

## TODOs

- Discuss in Stride Session: Is encryption needed for bluetooth comm, or is MAC with counter sufficient?
- Implement storage of maximum speed acc speed once ACC is turned on. (Lorenzo)
- Implement turning off ACC if speed gets below 30 kmH (Lorenzo)
- Business logic in GUI: If acc switches from failed to operable state, the state shall be always ("off") (instead of the state of the acc button). (Lorenzo)
- move initializers of members into MainWindow constructor. (Lorenzo)
- Remove obsolete state variables in MainWindow (all state is now kept in global variable, protected with a lock) (Lorenzo)
- Software design: Decouple "business logic" of our app from concrete HW to make it testable. (All three, if time available)
- Implement proper thread cleanup in both nodes. (Ernst)
- Create services that autostart node 1 and node 2 apps. (Ask whether this is required)
- Run MISRA checks on our code using cppcheck. (Ernst)
- Use proper data types in GUI: uint8_t for speed, uint16_t for distance (avoid signed data types) (Lorenzo)
- Clarify WLAN router issue in lab (Stefan)
