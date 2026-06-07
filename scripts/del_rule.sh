#!/bin/sh

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

if [ $# -ne 2 ]; then
    echo "用法：./del_rule.sh <INPUT|OUTPUT> <规则编号>"
    exit 1
fi

CHAIN=$1
RULE_NUM=$2

case "$CHAIN" in
    INPUT|OUTPUT)
        ;;
    *)
        echo "错误：方向只能是 INPUT 或 OUTPUT"
        exit 1
        ;;
esac

if [ "$(id -u)" -eq 0 ]; then
    iptables -D "$CHAIN" "$RULE_NUM"
elif command -v sudo >/dev/null 2>&1; then
    sudo iptables -D "$CHAIN" "$RULE_NUM"
else
    echo "错误：需要 root 权限执行 iptables"
    exit 1
fi
