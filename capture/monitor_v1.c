#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <net/ethernet.h>

// v1 版本：逐包打印源 IP、目的 IP、协议类型和包长度。
void packet_handler(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {
    // 原始 packet 的开头是以太网帧头。
    struct ether_header *eth = (struct ether_header *)packet;

    // 只处理 IPv4 数据包，其他类型直接跳过。
    if (ntohs(eth->ether_type) != ETHERTYPE_IP) {
        return;
    }

    // 跳过以太网头，继续解析 IP 头。
    struct ip *ip_header = (struct ip *)(packet + sizeof(struct ether_header));

    char src_ip[INET_ADDRSTRLEN];
    char dst_ip[INET_ADDRSTRLEN];

    strcpy(src_ip, inet_ntoa(ip_header->ip_src));
    strcpy(dst_ip, inet_ntoa(ip_header->ip_dst));

    const char *proto = "OTHER";

    // 根据 IP 头中的协议字段判断上层协议。
    if (ip_header->ip_p == IPPROTO_TCP) {
        proto = "TCP";
    } else if (ip_header->ip_p == IPPROTO_UDP) {
        proto = "UDP";
    } else if (ip_header->ip_p == IPPROTO_ICMP) {
        proto = "ICMP";
    }

    printf("%-15s -> %-15s | %-5s | bytes: %d\n",
           src_ip,
           dst_ip,
           proto,
           header->len);
}

int main(int argc, char *argv[]) {
    char errbuf[PCAP_ERRBUF_SIZE];
    char *device;

    // 支持手动指定网卡，例如 sudo ./monitor_v1 eth0。
    if (argc >= 2) {
        device = argv[1];
    } else {
        device = pcap_lookupdev(errbuf);
        if (device == NULL) {
            fprintf(stderr, "Cannot find default device: %s\n", errbuf);
            return 1;
        }
    }

    printf("Using device: %s\n", device);

    pcap_t *handle = pcap_open_live(device, BUFSIZ, 1, 1000, errbuf);

    if (handle == NULL) {
        fprintf(stderr, "Cannot open device %s: %s\n", device, errbuf);
        return 1;
    }

    printf("Start capturing packets...\n");

    // 每抓到一个包，libpcap 就调用 packet_handler。
    pcap_loop(handle, -1, packet_handler, NULL);

    pcap_close(handle);
    return 0;
}
