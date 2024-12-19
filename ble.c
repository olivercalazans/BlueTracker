// MIT License
// Copyright (c) 2024 Oliver Ribeiro Calazans Jeronimo
// Repository: https://github.com/olivercalazans/BlueTracker
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software...


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>


// Prototype declaration -------------------------------------------------------------------------------------
void handle_advertising_data();


// Colors ----------------------------------------------------------------------------------------------------
const char *RESET  = "\033[0m";
const char *GREEN  = "\033[32m";
const char *RED    = "\033[31m";
const char *YELLOW = "\033[33m";



int main() {
    int dev_id, sock, len;
    uint8_t buf[HCI_MAX_EVENT_SIZE];
    struct hci_filter original_filter, new_filter;

    dev_id = hci_get_route(NULL);
    sock = hci_open_dev(dev_id);

    if (dev_id < 0 || sock < 0) {
        perror("Failed to open Bluetooth device");
        exit(1);
    }

    struct hci_request scan_req;
    le_set_scan_parameters_cp scan_params = { 0x01, htobs(0x10), htobs(0x10), 0x00, 0x00 };
    memset(&scan_req, 0, sizeof(scan_req));
    scan_req.ogf = OGF_LE_CTL;
    scan_req.ocf = OCF_LE_SET_SCAN_PARAMETERS;
    scan_req.cparam = &scan_params;
    scan_req.clen = sizeof(scan_params);
    scan_req.rparam = NULL;
    scan_req.rlen = 0;

    if (hci_send_req(sock, &scan_req, 1000) < 0) {
        perror("Failed to set scan parameters");
        close(sock);
        exit(1);
    }

    le_set_scan_enable_cp scan_enable_cp = { 0x01, 0x00 };

    struct hci_request enable_req;
    memset(&enable_req, 0, sizeof(enable_req));
    enable_req.ogf = OGF_LE_CTL;
    enable_req.ocf = OCF_LE_SET_SCAN_ENABLE;
    enable_req.cparam = &scan_enable_cp;
    enable_req.clen = sizeof(scan_enable_cp);

    if (hci_send_req(sock, &enable_req, 1000) < 0) {
        perror("Failed to start scanning");
        close(sock);
        exit(1);
    }

    socklen_t original_filter_len = sizeof(original_filter);
    if (getsockopt(sock, SOL_HCI, HCI_FILTER, &original_filter, &original_filter_len) < 0) {
        perror("Could not get original HCI filter");
        close(sock);
        exit(1);
    }

    hci_filter_clear(&new_filter);
    hci_filter_set_ptype(HCI_EVENT_PKT, &new_filter);
    hci_filter_set_event(EVT_LE_META_EVENT, &new_filter);

    if (setsockopt(sock, SOL_HCI, HCI_FILTER, &new_filter, sizeof(new_filter)) < 0) {
        perror("Could not set HCI filter");
        close(sock);
        exit(1);
    }

    printf("Scanning BLE devices continuously...\n");

    while (1) {
        len = read(sock, buf, sizeof(buf));
        if (len < 0) {
            perror("Error reading HCI events");
            break;
        }

        evt_le_meta_event *meta_event = (evt_le_meta_event *)(buf + (1 + HCI_EVENT_HDR_SIZE));
        if (meta_event->subevent != EVT_LE_ADVERTISING_REPORT)
            continue;

        le_advertising_info *info = (le_advertising_info *)(meta_event->data + 1);
        char addr[18];
        ba2str(&info->bdaddr, addr);

        printf("Device: %s, RSSI: %d\n", addr, (int8_t)info->data[info->length]);

        handle_advertising_data(info->data, info->length);
    }

    setsockopt(sock, SOL_HCI, HCI_FILTER, &original_filter, sizeof(original_filter));

    close(sock);
    return 0;
}


void handle_advertising_data(uint8_t *data, int length) {
    printf("Raw advertising data: ");
    for (int i = 0; i < length; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}
