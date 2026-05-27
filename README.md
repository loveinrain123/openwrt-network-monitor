# openwrt-network-monitor
基于 OpenWrt 与 iptables 的轻量级流量控制与网络监测 Web Server
由于是在linux环境下编译运行，首先需要配置linux环境和所需要的工具链；
由于openwrt环境的不便利，可以采用交叉开发环境即WSL 主开发 + OpenWrt 最小验证
在 WSL 中写代码、编译、跑 Web；最后把程序或脚本放到 OpenWrt 环境中验证一次。
