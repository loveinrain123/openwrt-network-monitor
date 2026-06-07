#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <net/ethernet.h>

// 最多记录的不同 IP 数量。实验环境中 100 个已经足够演示。
#define MAX_IPS 100

// 保存单个 IP 的流量统计结果。
typedef struct {
    char ip[INET_ADDRSTRLEN];

    // tx 表示该 IP 作为源地址时发送的累计字节数。
    unsigned long tx_bytes;
    // rx 表示该 IP 作为目的地址时接收的累计字节数。
    unsigned long rx_bytes;

    unsigned long packets;

    // 记录该 IP 相关数据包中出现过的最大单包长度。
    unsigned int peak_packet_size;
} TrafficStats;

// 使用固定数组保存统计项，避免在抓包回调中频繁动态分配内存。
TrafficStats stats[MAX_IPS];

int ip_count = 0;
const char *json_output_path = "../backend/traffic.json";

// 查找某个 IP 的统计项；如果不存在，则创建一条新的统计项。
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

// 将当前统计结果打印到终端，便于命令行实时观察。
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

// 将统计结果写成 JSON 文件，Flask 后端会读取这个文件并返回给前端。
void write_json() {
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

void packet_handler(
    u_char *args,
    const struct pcap_pkthdr *header,
    const u_char *packet
) {

    // packet 是 libpcap 捕获到的原始字节流，开头是以太网帧头。
    struct ether_header *eth =
        (struct ether_header *)packet;

    // 只处理 IPv4 数据包，忽略 ARP、IPv6 等其他类型。
    if (ntohs(eth->ether_type) != ETHERTYPE_IP) {
        return;
    }

    // 跳过以太网头后，后面的内容就是 IP 头。
    struct ip *ip_header =
        (struct ip *)(packet + sizeof(struct ether_header));

    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];

    strcpy(src_ip, inet_ntoa(ip_header->ip_src));
    strcpy(dst_ip, inet_ntoa(ip_header->ip_dst));

    int src_index = find_or_create_ip(src_ip);
    int dst_index = find_or_create_ip(dst_ip);

    // 源 IP 视为发送方，累计发送字节数。
    if (src_index >= 0) {

        stats[src_index].tx_bytes += header->len;

        stats[src_index].packets++;

        if (header->len > stats[src_index].peak_packet_size) {

            stats[src_index].peak_packet_size = header->len;
        }
    }

    // 目的 IP 视为接收方，累计接收字节数。
    if (dst_index >= 0) {

        stats[dst_index].rx_bytes += header->len;

        stats[dst_index].packets++;

        if (header->len > stats[dst_index].peak_packet_size) {

            stats[dst_index].peak_packet_size = header->len;
        }
    }

    system("clear");

    print_stats();

    write_json();
}

int main(int argc, char *argv[]) {

    char errbuf[PCAP_ERRBUF_SIZE];

    char *device;

    // 支持通过命令行指定网卡，例如：sudo ./monitor eth0。
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

    // 第二个参数可指定 JSON 输出路径，便于部署到 OpenWrt 后写入固定位置。
    if (argc >= 3) {
        json_output_path = argv[2];
    }

    printf("Using device: %s\n", device);
    printf("JSON output: %s\n", json_output_path);

    // 打开网卡进行实时抓包：1 表示混杂模式，1000 表示超时时间 1000ms。
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

    // -1 表示一直抓包；每抓到一个包，就调用 packet_handler。
    pcap_loop(handle,
              -1,
              packet_handler,
              NULL);

    pcap_close(handle);

    return 0;
}
