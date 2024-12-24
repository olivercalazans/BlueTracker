// MIT License
// Copyright (c) 2024 Oliver Ribeiro Calazans Jeronimo
// Repository: https://github.com/olivercalazans/BlueTracker
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software...


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>


// Prototype declaration -------------------------------------------------------------------------------------
void verify_if_the_bluetooth_can_be_used(int *dev_id, int *sock);
void set_scan_parameters(int *sock);
void start_scan(int *sock);
void fetch_hci_filter(int *sock, struct hci_filter *original_filter);
void clear_filter(struct hci_filter *filter);
void set_event_filter(struct hci_filter *filter);
void set_packet_type_filter(struct hci_filter *filter);
void apply_filter(int *sock, struct hci_filter *filter);
void handle_advertising_data(uint8_t *data, int length);
char* get_distances(int8_t *rssi);
double calculate_distance(int8_t *rssi, int *path_loss_exponent);



int main() {
    int dev_id, sock, len;
    uint8_t buffer[HCI_MAX_EVENT_SIZE];
    struct hci_filter original_filter, new_filter;

    dev_id = hci_get_route(NULL);
    sock   = hci_open_dev(dev_id);
    
    verify_if_the_bluetooth_can_be_used(&dev_id, &sock);
    set_scan_parameters(&sock);
    start_scan(&sock);
    fetch_hci_filter(&sock, &original_filter);
    clear_filter(&new_filter);
    set_event_filter(&new_filter);
    set_packet_type_filter(&new_filter);
    apply_filter(&sock, &new_filter);


    printf("Scanning BLE devices continuously...\n");

    while (1) {
        len = read(sock, buffer, sizeof(buffer));
        if (len < 0) {
            perror("Error reading HCI events");
            break;
        }

        evt_le_meta_event *meta_event = (evt_le_meta_event *)(buffer + (1 + HCI_EVENT_HDR_SIZE));
        if (meta_event->subevent != EVT_LE_ADVERTISING_REPORT)
            continue;

        le_advertising_info *info = (le_advertising_info *)(meta_event->data + 1);
        char addr[18];
        ba2str(&info->bdaddr, addr);

        int8_t rssi    = (int8_t)info->data[info->length];
        char *distance = get_distances(&rssi);

        printf("Device: %s, Estimated Distance: %s meters\n", addr, distance);

        handle_advertising_data(info->data, info->length);
    }

    setsockopt(sock, SOL_HCI, HCI_FILTER, &original_filter, sizeof(original_filter));

    close(sock);
    return 0;
}



void verify_if_the_bluetooth_can_be_used(int *dev_id, int *sock) {
    if (*dev_id < 0 || *sock < 0){
        perror("Failed to open Bluetooth device");
        exit(1);
    }
}



void set_scan_parameters(int *sock) {
    struct hci_request scan_req;
    le_set_scan_parameters_cp scan_params = { 0x01, htobs(0x10), htobs(0x10), 0x00, 0x00 };
    memset(&scan_req, 0, sizeof(scan_req));
    scan_req.ogf    = OGF_LE_CTL;
    scan_req.ocf    = OCF_LE_SET_SCAN_PARAMETERS;
    scan_req.cparam = &scan_params;
    scan_req.clen   = sizeof(scan_params);
    scan_req.rparam = NULL;
    scan_req.rlen   = 0;

    if (hci_send_req(*sock, &scan_req, 1000) < 0) {
        perror("Failed to set scan parameters");
        close(*sock);
        exit(1);
    }
}



void start_scan(int *sock) {
    le_set_scan_enable_cp scan_enable_cp = { 0x01, 0x00 };

    struct hci_request enable_req;
    memset(&enable_req, 0, sizeof(enable_req));
    enable_req.ogf    = OGF_LE_CTL;
    enable_req.ocf    = OCF_LE_SET_SCAN_ENABLE;
    enable_req.cparam = &scan_enable_cp;
    enable_req.clen   = sizeof(scan_enable_cp);

    if (hci_send_req(*sock, &enable_req, 1000) < 0) {
        perror("Failed to start scanning");
        close(*sock);
        exit(1);
    }
}



void fetch_hci_filter(int *sock, struct hci_filter *original_filter) {
    socklen_t original_filter_len = sizeof(*original_filter);
    if (getsockopt(*sock, SOL_HCI, HCI_FILTER, original_filter, &original_filter_len) < 0) {
        perror("Could not get original HCI filter");
        close(*sock);
        exit(1);
    }
}



void clear_filter(struct hci_filter *filter) {
    hci_filter_clear(filter);
}

void set_event_filter(struct hci_filter *filter) {
    hci_filter_set_event(EVT_LE_META_EVENT, filter);
}

void set_packet_type_filter(struct hci_filter *filter) {
    hci_filter_set_ptype(HCI_EVENT_PKT, filter);
}



void apply_filter(int *sock, struct hci_filter *filter) {
    if (setsockopt(*sock, SOL_HCI, HCI_FILTER, filter, sizeof(*filter)) < 0) {
        perror("Could not set HCI filter");
        close(*sock);
        exit(1);
    }
}



char* get_distances(int8_t *rssi) {
    static char result[40];
    result[0] = '\0';

    for (int i = 2; i <= 4; i += 2) {
        double distance = calculate_distance(rssi, &i);

        if (distance > 10) {
            snprintf(result + strlen(result), sizeof(result) - strlen(result), "\033[31m%.2f\033[0m", distance); // Red
        } else if (distance >= 5) {
            snprintf(result + strlen(result), sizeof(result) - strlen(result), "\033[33m%.2f\033[0m", distance); // Yellow
        } else {
            snprintf(result + strlen(result), sizeof(result) - strlen(result), "\033[32m%.2f\033[0m", distance); // Green
        }

        if (i == 2) {
            strcat(result, " ~ ");
        }
    }

    return result;
}



double calculate_distance(int8_t *rssi, int *path_loss_exponent) {
    return pow(10.0, (-50 - *rssi) / (10.0 * *path_loss_exponent));
}



void handle_advertising_data(uint8_t *data, int length) {
    printf("Raw advertising data: ");
    for (int i = 0; i < length; i++) {
        printf("%02x ", data[i]);
    }
    printf("\n");
}
