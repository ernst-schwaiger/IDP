#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    struct sockaddr_l2 addr = { 0 };
    int s, status;
    char dest[18] = "DC:A6:32:BB:79:EF";

    if(argc != 3)
    {
        fprintf(stderr, "usage: %s <bt_mac_addr> <msg>\n", argv[0]);
        exit(2);
    }

    strncpy(dest, argv[1], 18);

    // allocate a socket
    s = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

    // set the connection parameters (who to connect to)
    addr.l2_family = AF_BLUETOOTH;
    addr.l2_psm = htobs(0x1001);
    str2ba( dest, &addr.l2_bdaddr );

    // connect to server
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

    // send a message
    if( status == 0 )
    {
        status = write(s, argv[2], strlen(argv[2]));
    }

    if( status < 0 )
    {
        perror("uh oh");
    }

    close(s);
}
