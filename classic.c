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
    inquiry_info *info = NULL;
    int max_rsp, num_rsp;
    int dev_id, sock, len, flags;
    char addr[19] = { 0 };
    char name[248] = { 0 };

    dev_id = hci_get_route(NULL);
    sock = hci_open_dev(dev_id);

    if (dev_id < 0 || sock < 0) {
        perror("Failed to open Bluetooth device");
        exit(1);
    }

    len = 8;
    max_rsp = 255;
    flags = IREQ_CACHE_FLUSH;

    info = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &info, flags);

    if (num_rsp < 0) {
        perror("Failed to perform inquiry");
    }

    for (int i = 0; i < num_rsp; i++) {
        ba2str(&(info[i].bdaddr), addr);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(sock, &(info[i].bdaddr), sizeof(name), name, 0) < 0) {
            strcpy(name, "[unknown]");
        }
        printf("Device: %s, Name: %s\n", addr, name);
    }

    free(info);
    close(sock);
    return 0;
}
