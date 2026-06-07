#!/bin/sh

ACTION=$1
PROTO=$2
PORT=$3
CHAIN=${4:-INPUT}

# OpenWrt 通常以 root 执行；WSL 普通用户执行时自动使用 sudo。
run_iptables() {
    if [ "$(id -u)" -eq 0 ]; then
        iptables "$@"
    elif command -v sudo >/dev/null 2>&1; then
        sudo iptables "$@"
    else
        echo "错误：需要 root 权限执行 iptables"
        exit 1
    fi
}

case "$ACTION" in
    add)
        # 添加端口 DROP 规则：
        # INPUT 用于限制外部访问本机，OUTPUT 用于限制本机访问外网。
        case "$CHAIN" in
            INPUT|OUTPUT)
                run_iptables -A "$CHAIN" -p "$PROTO" --dport "$PORT" -j DROP
                echo "已添加规则：在 $CHAIN 链禁止 $PROTO 端口 $PORT"
                ;;
            *)
                echo "错误：方向只能是 INPUT 或 OUTPUT"
                exit 1
                ;;
        esac
        ;;

    blockip)
        # 封禁本机访问指定目标 IP。
        IP=$2
        run_iptables -A OUTPUT -d "$IP" -j DROP
        echo "已封禁 IP：$IP"
        ;;

    list)
        # 查看当前防火墙规则，并显示规则序号。
        run_iptables -L -n --line-numbers
        ;;

    clear)
        # 清空规则，便于实验结束后恢复网络。
        run_iptables -F
        echo "已清空所有防火墙规则"
        ;;

    *)
        echo "用法：./firewall.sh add tcp 80 [INPUT|OUTPUT] | blockip 8.8.8.8 | list | clear"
        ;;
esac
