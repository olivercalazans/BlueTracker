// MIT License
// Copyright (c) 2024 Oliver Ribeiro Calazans Jeronimo
// Repository: https://github.com/olivercalazans/BlueTracker
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software...


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

int main() {
    int dev_id, sock;
    struct hci_filter old_filter, new_filter;
    unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
    int len;

    dev_id = hci_get_route(NULL);
    sock = hci_open_dev(dev_id);

    if (dev_id < 0 || sock < 0) {
        perror("Failed to open Bluetooth device");
        exit(1);
    }

    hci_le_set_scan_parameters(sock, 0x01, htobs(0x0010), htobs(0x0010), 0x00, 0x00, 1000);
    hci_le_set_scan_enable(sock, 0x01, 1, 1000);

    printf("Scanning BLE devices...\n");
    while (1) {
        len = read(sock, buf, sizeof(buf));
        if (len > 0) {
            ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
            len -= (1 + HCI_EVENT_HDR_SIZE);
            printf("BLE device found: %s\n", ptr);
        }
    }

    hci_le_set_scan_enable(sock, 0x00, 1, 1000);
    close(sock);

    return 0;
}
