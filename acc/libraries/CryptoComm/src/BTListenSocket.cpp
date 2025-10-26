#include "BTListenSocket.h"
#include "BTRuntimeError.h"

#include <stdexcept>
#include <gio/gio.h>
#include <glib.h>

using namespace std;

namespace acc
{

BTListenSocket::BTListenSocket(void) : 
    m_local_addr{ AF_BLUETOOTH, htobs(0x1001), { 0 }, 0, 0 }
{
    makeBTDeviceVisible();
    m_listenSocket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

    if (m_listenSocket < 0)
    {
        throw BTRuntimeError("failed to create listen socket");
    }

    if (::bind(m_listenSocket, reinterpret_cast<struct sockaddr *>(&m_local_addr), sizeof(m_local_addr)) < 0)
    {
        throw BTRuntimeError("failed to bind to listen socket");
    }

    // put socket into listening mode
    ::listen(m_listenSocket, 1);
}

BTListenSocket::~BTListenSocket(void)
{
    close(m_listenSocket);
}

// This is the same as "bluetoothctrl discoverable yes"
void BTListenSocket::makeBTDeviceVisible(void)
{
    GError *error = nullptr;
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (error != nullptr)
    {
        throw BTRuntimeError(error->message);
    }

    GDBusProxy *adapter_proxy = g_dbus_proxy_new_sync(
        connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "org.bluez",
        "/org/bluez/hci0",
        "org.bluez.Adapter1",
        NULL,
    &error);

    if (error != nullptr)
    {
        throw BTRuntimeError(error->message);
    }

    GVariant *value = g_variant_new_boolean(TRUE);

    g_dbus_proxy_set_cached_property(adapter_proxy, "Discoverable", value);

    g_dbus_connection_call_sync(
        connection,
        "org.bluez",                        // Bus name
        "/org/bluez/hci0",                 // Object path
        "org.freedesktop.DBus.Properties", // Interface
        "Set",                              // Method
        g_variant_new("(ssv)", 
            "org.bluez.Adapter1",           // Interface name
            "Discoverable",                 // Property name
            g_variant_new_boolean(TRUE)),   // Value
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);    

    if (error != nullptr)
    {
        throw BTRuntimeError(error->message);
    }
}

}
