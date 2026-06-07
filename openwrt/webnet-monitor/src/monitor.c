#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <net/ethernet.h>

#define MAX_IPS 100

typedef struct {
    char ip[INET_ADDRSTRLEN];
    unsigned long tx_bytes;
    unsigned long rx_bytes;
    unsigned long packets;
    unsigned int peak_packet_size;
} TrafficStats;

static TrafficStats stats[MAX_IPS];
static int ip_count = 0;
static const char *json_output_path = "/tmp/webnet/backend/traffic.json";

static int find_or_create_ip(const char *ip) {
    for (int i = 0; i < ip_count; i++) {
        if (strcmp(stats[i].ip, ip) == 0) {
            return i;
        }
    }

    if (ip_count >= MAX_IPS) {
        return -1;
    }

    strcpy(stats[ip_count].ip, ip);
    stats[ip_count].tx_bytes = 0;
    stats[ip_count].rx_bytes = 0;
    stats[ip_count].packets = 0;
    stats[ip_count].peak_packet_size = 0;
    ip_count++;

    return ip_count - 1;
}

static void print_stats(void) {
    printf("\033[2J\033[H");
    printf("====================================================\n");
    printf("%-16s %-12s %-12s %-10s %-10s\n",
           "IP", "TX(Bytes)", "RX(Bytes)", "Packets", "Peak");
    printf("----------------------------------------------------\n");

    for (int i = 0; i < ip_count; i++) {
        printf("%-16s %-12lu %-12lu %-10lu %-10u\n",
               stats[i].ip,
               stats[i].tx_bytes,
               stats[i].rx_bytes,
               stats[i].packets,
               stats[i].peak_packet_size);
    }

    printf("====================================================\n");
    fflush(stdout);
}

static void write_json(void) {
    FILE *fp = fopen(json_output_path, "w");
    if (fp == NULL) {
        return;
    }

    fprintf(fp, "[\n");
    for (int i = 0; i < ip_count; i++) {
        fprintf(fp,
                "  {\"ip\":\"%s\", \"tx\":%lu, \"rx\":%lu, \"packets\":%lu, \"peak\":%u}%s\n",
                stats[i].ip,
                stats[i].tx_bytes,
                stats[i].rx_bytes,
                stats[i].packets,
                stats[i].peak_packet_size,
                i == ip_count - 1 ? "" : ",");
    }
    fprintf(fp, "]\n");
    fclose(fp);
}

static void packet_handler(u_char *args,
                           const struct pcap_pkthdr *header,
                           const u_char *packet) {
    (void)args;

    const struct ether_header *eth = (const struct ether_header *)packet;
    if (ntohs(eth->ether_type) != ETHERTYPE_IP) {
        return;
    }

    const struct ip *ip_header =
        (const struct ip *)(packet + sizeof(struct ether_header));

    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];

    strcpy(src_ip, inet_ntoa(ip_header->ip_src));
    strcpy(dst_ip, inet_ntoa(ip_header->ip_dst));

    int src_index = find_or_create_ip(src_ip);
    int dst_index = find_or_create_ip(dst_ip);

    if (src_index >= 0) {
        stats[src_index].tx_bytes += header->len;
        stats[src_index].packets++;
        if (header->len > stats[src_index].peak_packet_size) {
            stats[src_index].peak_packet_size = header->len;
        }
    }

    if (dst_index >= 0) {
        stats[dst_index].rx_bytes += header->len;
        stats[dst_index].packets++;
        if (header->len > stats[dst_index].peak_packet_size) {
            stats[dst_index].peak_packet_size = header->len;
        }
    }

    print_stats();
    write_json();
}

int main(int argc, char *argv[]) {
    char errbuf[PCAP_ERRBUF_SIZE];
    char *device;

    if (argc >= 2) {
        device = argv[1];
    } else {
        device = pcap_lookupdev(errbuf);
        if (device == NULL) {
            fprintf(stderr, "Cannot find default device: %s\n", errbuf);
            return 1;
        }
    }

    if (argc >= 3) {
        json_output_path = argv[2];
    }

    printf("Using device: %s\n", device);
    printf("JSON output: %s\n", json_output_path);

    pcap_t *handle = pcap_open_live(device, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Cannot open device %s: %s\n", device, errbuf);
        return 1;
    }

    printf("Start capturing packets...\n");
    pcap_loop(handle, -1, packet_handler, NULL);

    pcap_close(handle);
    return 0;
}
