#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <net/ethernet.h>

// v2 版本：在 v1 的逐包解析基础上，增加按 IP 聚合统计。
#define MAX_IPS 100

// 保存单个 IP 的累计流量信息。
typedef struct {
    char ip[INET_ADDRSTRLEN];

    unsigned long tx_bytes;
    unsigned long rx_bytes;

    unsigned long packets;

    unsigned int peak_packet_size;
} TrafficStats;

TrafficStats stats[MAX_IPS];

int ip_count = 0;

// 查找某个 IP；如果首次出现，则创建新的统计项。
int find_or_create_ip(const char *ip) {

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

// 在终端打印当前所有 IP 的统计表。
void print_stats() {

    printf("\n====================================================\n");

    printf("%-16s %-12s %-12s %-10s %-10s\n",
           "IP",
           "TX(Bytes)",
           "RX(Bytes)",
           "Packets",
           "Peak");

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
}

void packet_handler(
    u_char *args,
    const struct pcap_pkthdr *header,
    const u_char *packet
) {

    // 把原始数据包开头解释为以太网帧头。
    struct ether_header *eth =
        (struct ether_header *)packet;

    // 只统计 IPv4 数据包。
    if (ntohs(eth->ether_type) != ETHERTYPE_IP) {
        return;
    }

    // 跳过以太网头后解析 IP 头。
    struct ip *ip_header =
        (struct ip *)(packet + sizeof(struct ether_header));

    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];

    strcpy(src_ip, inet_ntoa(ip_header->ip_src));
    strcpy(dst_ip, inet_ntoa(ip_header->ip_dst));

    int src_index = find_or_create_ip(src_ip);
    int dst_index = find_or_create_ip(dst_ip);

    // 源 IP 作为发送方，累加 TX。
    if (src_index >= 0) {

        stats[src_index].tx_bytes += header->len;

        stats[src_index].packets++;

        if (header->len > stats[src_index].peak_packet_size) {

            stats[src_index].peak_packet_size = header->len;
        }
    }

    // 目的 IP 作为接收方，累加 RX。
    if (dst_index >= 0) {

        stats[dst_index].rx_bytes += header->len;

        stats[dst_index].packets++;

        if (header->len > stats[dst_index].peak_packet_size) {

            stats[dst_index].peak_packet_size = header->len;
        }
    }

    system("clear");

    print_stats();
}

int main(int argc, char *argv[]) {

    char errbuf[PCAP_ERRBUF_SIZE];

    char *device;

    // 支持手动指定网卡，例如 sudo ./monitor_v2 eth0。
    if (argc >= 2) {

        device = argv[1];

    } else {

        device = pcap_lookupdev(errbuf);

        if (device == NULL) {

            fprintf(stderr,
                    "Cannot find default device: %s\n",
                    errbuf);

            return 1;
        }
    }

    printf("Using device: %s\n", device);

    pcap_t *handle =
        pcap_open_live(device,
                       BUFSIZ,
                       1,
                       1000,
                       errbuf);

    if (handle == NULL) {

        fprintf(stderr,
                "Cannot open device %s: %s\n",
                device,
                errbuf);

        return 1;
    }

    printf("Start capturing packets...\n");

    // 持续抓包，并在回调中更新统计表。
    pcap_loop(handle,
              -1,
              packet_handler,
              NULL);

    pcap_close(handle);

    return 0;
}
