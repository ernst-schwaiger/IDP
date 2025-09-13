# Bluetooth Getting Started

- Install the bluez bluetooth stack, bluetooth library: `sudo apt install bluez bluetoothlib-dev`
- Status of bluetooth daemon `systemctl status bluetooth`
- Get MAC addresses of local bluetooth controllers: `bluetoothctl list`
- Bluetooth monitor for debugging: `sudo btmon`
- Demo client/server bluetooth app sending a message from a client to a server, taken from https://people.csail.mit.edu/albert/bluez-intro/x559.html

## Run the Demo

- Ensure on both nodes the bluetooth service is up and running: `systemctl status bluetooth`, start if required `sudo systemctl start bluetooth`
- On both nodes, run `make`, compiling the `server` and `client` binaries
- On the server node, obtain the MAC address: `bluetoothctl list`
- On the server node, make the bluetooth controller discoverable: `bluetoothctl discoverable yes` 
- On the server node, run `./server`
- On the client node, run `./client <server_mac_address> <message>`
- The server should print out the message
