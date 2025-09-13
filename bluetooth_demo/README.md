# Bluetooth Getting Started

- Install `bluez` bluetooth stack, bluetooth library: `sudo apt install bluez bluetoothlib-dev`
- Status of bluetooth daemon `systemctl status bluetooth`
- Get MAC addresses of local bluetooth controllers: `bluetoothctl list`

- Bluetooth monitor for debugging: `sudo btmon`
- Demo client/server bluetooth app sending a message from a client to a server, taken from https://people.csail.mit.edu/albert/bluez-intro/x559.html
