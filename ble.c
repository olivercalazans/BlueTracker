#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <time.h>


#define MAX_DEVICES 100
#define SCAN_INTERVAL 1


typedef struct {
    char addr[18];           // Device address
    int8_t rssi;             // Device RSSI
    char distance[25];       // Estimated distance
    double numeric_distance; // Numeric distance for ordering
} Device;


void verify_if_the_bluetooth_can_be_used(int *dev_id, int *sock);
void set_scan_parameters(int *sock);
void start_scan(int *sock);
void fetch_hci_filter(int *sock, struct hci_filter *original_filter);
void clear_filter(struct hci_filter *filter);
void set_event_filter(struct hci_filter *filter);
void set_packet_type_filter(struct hci_filter *filter);
void apply_filter(int *sock, struct hci_filter *filter);
char* get_distances(int8_t *rssi, double *numeric_distance);
double calculate_distance(int8_t *rssi);
void verify_if_there_is_data(int *len);
void add_device(Device devices[], int *device_count, const char *addr, int8_t rssi);
int compare_devices(const void *a, const void *b);
int verify_if_a_device_exists(Device devices[], int device_count, const char *addr);
void clear_screen_and_print_devices(Device devices[], int *device_count);



int main() {
    int dev_id, sock, len;
    uint8_t buffer[HCI_MAX_EVENT_SIZE];
    struct hci_filter original_filter, new_filter;
    Device devices[MAX_DEVICES];
    int device_count = 0;
    
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
        time_t start_time = time(NULL);
        device_count = 0;
        
        while (difftime(time(NULL), start_time) < SCAN_INTERVAL) {
            len = read(sock, buffer, sizeof(buffer));
            verify_if_there_is_data(&len);
            
            evt_le_meta_event *meta_event = (evt_le_meta_event *)(buffer + (1 + HCI_EVENT_HDR_SIZE));
            if (meta_event->subevent != EVT_LE_ADVERTISING_REPORT)
                continue;

            le_advertising_info *info = (le_advertising_info *)(meta_event->data + 1);
            char addr[18];
            ba2str(&info->bdaddr, addr);

            int8_t rssi = (int8_t)info->data[info->length];
            double numeric_distance;
            char *distance = get_distances(&rssi, &numeric_distance);

            // Adds the device to the list if it is not already in the list
            if (!verify_if_a_device_exists(devices, device_count, addr)) {
                add_device(devices, &device_count, addr, rssi);
            }
        }

        // Sort devices in descending order of distance
        qsort(devices, device_count, sizeof(Device), compare_devices);
        
        // Clears the screen and prints devices captured during the interval
        clear_screen_and_print_devices(devices, &device_count);
        
        // Wait 0.5 seconds before starting the next cycle
        usleep(500000);
    }

    setsockopt(sock, SOL_HCI, HCI_FILTER, &original_filter, sizeof(original_filter));
    close(sock);
    return 0;
}



void verify_if_the_bluetooth_can_be_used(int *dev_id, int *sock) {
    if (*dev_id < 0 || *sock < 0) {
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



char* get_distances(int8_t *rssi, double *numeric_distance) {
    static char result[25];
    *numeric_distance = calculate_distance(rssi);

    if (*numeric_distance > 10) {
        snprintf(result, sizeof(result), "\033[31m%.2f\033[0m", *numeric_distance); // Red
    } else if (*numeric_distance >= 5) {
        snprintf(result, sizeof(result), "\033[33m%.2f\033[0m", *numeric_distance); // Yellow
    } else {
        snprintf(result, sizeof(result), "\033[32m%.2f\033[0m", *numeric_distance); // Green
    }
    return result;
}



double calculate_distance(int8_t *rssi) {
    return pow(10.0, (-50 - *rssi) / (10.0 * 3));
}



void verify_if_there_is_data(int *len) {
    if (*len < 0) {
        perror("Error reading HCI events");
        exit(1);
    }
}



void add_device(Device devices[], int *device_count, const char *addr, int8_t rssi) {
    if (*device_count >= MAX_DEVICES) {
        printf("Device list is full.\n");
        return;
    }

    strcpy(devices[*device_count].addr, addr);
    devices[*device_count].rssi = rssi;
    devices[*device_count].numeric_distance = calculate_distance(&rssi);
    strcpy(devices[*device_count].distance, get_distances(&rssi, &devices[*device_count].numeric_distance));
    (*device_count)++;
}



int verify_if_a_device_exists(Device devices[], int device_count, const char *addr) {
    for (int i = 0; i < device_count; i++) {
        if (strcmp(devices[i].addr, addr) == 0) {
            return 1;
        }
    }
    return 0;
}



int compare_devices(const void *a, const void *b) {
    Device *deviceA = (Device *)a;
    Device *deviceB = (Device *)b;

    if (deviceA->numeric_distance < deviceB->numeric_distance) return 1;
    if (deviceA->numeric_distance > deviceB->numeric_distance) return -1;
    return 0;
}



void clear_screen_and_print_devices(Device devices[], int *device_count) {
    system("clear");

    printf("Detected Devices:\n");
    printf("-----------------\n");

    for (int i = 0; i < *device_count; i++) {
        printf("Device: %s, Estimated Distance: %s meters\n", devices[i].addr, devices[i].distance);
    }

    *device_count = 0;
}
