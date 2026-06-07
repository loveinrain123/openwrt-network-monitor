# OpenWrt Traffic Monitor & Firewall Web

基于 OpenWrt、libpcap、Flask、Chart.js 和 iptables 的轻量级网络流量监控与防火墙规则管理系统。

本项目用于《计算机网络系统》实验二：基于 OpenWrt 的网络应用程序开发。系统支持在 WSL 中完成主要开发、编译和 Web 调试，也支持通过 OpenWrt SDK 交叉编译 `webnet-monitor`，并在 OpenWrt 虚拟机中运行流量监控程序和 Flask Web 服务。

## 功能特性

- 使用 C + libpcap 捕获网卡数据包。
- 按 IP 聚合统计发送流量、接收流量、数据包数量和峰值包大小。
- 将流量统计结果写入 `traffic.json`。
- 使用 Flask 提供 Web API 和静态页面服务。
- 使用 Chart.js 展示实时流量柱状图。
- 使用 Shell 脚本封装 iptables 防火墙规则。
- 支持添加 TCP/UDP 端口 DROP 规则。
- 支持选择 `INPUT` / `OUTPUT` 方向。
- 支持封禁指定目标 IP。
- 支持查看和清空当前 iptables 规则。
- 支持 OpenWrt SDK 交叉编译并部署到 OpenWrt。

## 项目结构

```text
program/
├── backend/
│   ├── app.py              # Flask 后端服务
│   └── traffic.json        # monitor 输出的流量统计数据
├── capture/
│   ├── test.c              # libpcap 环境测试
│   ├── monitor_v1.c        # 第一版：逐包打印 IP/协议/长度
│   ├── monitor_v2.c        # 第二版：按 IP 聚合统计
│   └── monitor.c           # 最终版：统计并输出 JSON
├── frontend/
│   ├── index.html          # 前端页面
│   ├── main.js             # 前端请求和交互逻辑
│   └── style.css           # 页面样式
├── scripts/
│   ├── firewall.sh         # iptables 规则管理脚本
│   ├── add_rule.sh
│   ├── del_rule.sh
│   └── list_rule.sh
├── openwrt/
│   ├── README-SDK.md       # OpenWrt SDK 交叉编译说明
│   └── webnet-monitor/     # OpenWrt SDK 包目录
└── docs/                   # 实验指导书、报告和截图材料
```

## 系统流程

```text
webnet-monitor / monitor.c
        ↓ 写入 JSON
backend/traffic.json
        ↓ Flask 读取
GET /api/traffic
        ↓ 前端 fetch
表格 + Chart.js 图表
```

防火墙规则管理流程：

```text
前端表单
    ↓ POST JSON
Flask API
    ↓ 参数校验
scripts/firewall.sh
    ↓
iptables
```

## WSL 开发运行

### 1. 安装依赖

```bash
sudo apt update
sudo apt install -y gcc libpcap-dev python3 python3-pip python3-venv iptables
```

Ubuntu 24.04 默认保护系统 Python 环境，建议使用虚拟环境：

```bash
cd /mnt/d/vibecodeing/webnet/program/backend
python3 -m venv .venv
source .venv/bin/activate
python -m pip install flask flask-cors
```

### 2. 编译并运行抓包程序

```bash
cd /mnt/d/vibecodeing/webnet/program/capture
gcc monitor.c -o monitor -lpcap
sudo ./monitor eth0 ../backend/traffic.json
```

如果网卡不是 `eth0`，先查看：

```bash
ip addr
```

### 3. 启动 Web 后端

另开一个 WSL 终端：

```bash
cd /mnt/d/vibecodeing/webnet/program/backend
source .venv/bin/activate
sudo .venv/bin/python app.py
```

浏览器访问：

```text
http://127.0.0.1:5000
```

### 4. 产生测试流量

```bash
ping 8.8.8.8
curl -I http://baidu.com
```

前端页面会自动刷新流量表格和图表。

## 防火墙功能验证

查看规则：

```bash
cd /mnt/d/vibecodeing/webnet/program
sudo scripts/firewall.sh list
```

添加 OUTPUT 方向 TCP 80 端口 DROP 规则：

```bash
sudo scripts/firewall.sh add tcp 80 OUTPUT
```

验证 HTTP 访问被阻断：

```bash
wget -q -O /dev/null -T 5 http://baidu.com
echo $?
```

返回非 `0` 表示访问失败。也可以查看 iptables 计数器：

```bash
sudo iptables -L OUTPUT -n -v --line-numbers
```

清空规则：

```bash
sudo scripts/firewall.sh clear
```

说明：

- `INPUT`：外部访问本机。
- `OUTPUT`：本机访问外部。
- `FORWARD`：经过路由器转发的流量。
- `ping` 使用 ICMP，不会被 `tcp dpt:80` 规则拦截。

## OpenWrt 部署

实验环境：

```text
OpenWrt 24.10.0 x86/64
IP: 192.168.31.194
Interface: eth0
```

### 1. 精简传输项目

OpenWrt 存储空间较小，不建议直接上传整个项目，尤其不要上传 `.venv` 和 `docs`。

在 WSL 中打包：

```bash
cd /mnt/d/vibecodeing/webnet

tar \
  --exclude='program/.git' \
  --exclude='program/backend/.venv' \
  --exclude='program/docs' \
  --exclude='program/backend/traffic.json' \
  --exclude='program/capture/monitor' \
  --exclude='program/capture/test' \
  -czf /tmp/webnet-lite.tar.gz program
```

上传到 OpenWrt：

```bash
scp /tmp/webnet-lite.tar.gz root@192.168.31.194:/tmp/
```

在 OpenWrt 中解压：

```sh
cd /tmp
tar -xzf webnet-lite.tar.gz
mv program webnet
cd /tmp/webnet
```

`/tmp` 是临时目录，重启后会清空，但适合实验演示。

### 2. 修正 OpenWrt Shell 解释器

OpenWrt 默认是 BusyBox `ash`，通常没有 `/bin/bash`。

```sh
sed -i 's/\r$//' scripts/firewall.sh
sed -i '1s|.*|#!/bin/sh|' scripts/firewall.sh
chmod +x scripts/*.sh
```

### 3. 验证 OpenWrt 防火墙

```sh
./scripts/firewall.sh clear
./scripts/firewall.sh add tcp 80 OUTPUT
iptables -L OUTPUT -n -v --line-numbers
```

测试 HTTP：

```sh
wget -q -O /dev/null -T 5 http://baidu.com
echo $?
```

清空恢复：

```sh
./scripts/firewall.sh clear
wget -q -O /dev/null -T 5 http://baidu.com
echo $?
```

## OpenWrt SDK 交叉编译

项目提供了 OpenWrt SDK 包：

```text
openwrt/webnet-monitor
```

用于在 WSL 中交叉编译 `webnet-monitor`，再安装到 OpenWrt。

### 1. 下载 SDK

```bash
cd ~
wget https://downloads.openwrt.org/releases/24.10.0/targets/x86/64/openwrt-sdk-24.10.0-x86-64_gcc-13.3.0_musl.Linux-x86_64.tar.zst
sudo apt install -y zstd build-essential rsync unzip file
tar --use-compress-program=unzstd -xf openwrt-sdk-24.10.0-x86-64_gcc-13.3.0_musl.Linux-x86_64.tar.zst
cd openwrt-sdk-24.10.0-x86-64_gcc-13.3.0_musl.Linux-x86_64
```

### 2. 安装 libpcap feed

```bash
./scripts/feeds update base
./scripts/feeds install -p base libpcap
```

### 3. 加入项目包

```bash
cp -r /mnt/d/vibecodeing/webnet/program/openwrt/webnet-monitor package/
```

配置并编译：

```bash
echo 'CONFIG_PACKAGE_libpcap=m' >> .config
echo 'CONFIG_PACKAGE_webnet-monitor=m' >> .config
make defconfig
make package/feeds/base/libpcap/compile V=s
make package/webnet-monitor/compile V=s
```

查找生成的安装包：

```bash
find bin -name '*webnet-monitor*.ipk'
find bin -name '*libpcap*.ipk'
```

### 4. 安装到 OpenWrt

```bash
scp bin/packages/x86_64/base/webnet-monitor_1.0-r1_x86_64.ipk root@192.168.31.194:/tmp/
scp $(find bin -name '*libpcap*.ipk' | head -n 1) root@192.168.31.194:/tmp/
```

在 OpenWrt 中安装：

```sh
opkg install /tmp/libpcap*.ipk
opkg install /tmp/webnet-monitor_1.0-r1_x86_64.ipk
```

运行：

```sh
mkdir -p /tmp/webnet/backend
webnet-monitor eth0 /tmp/webnet/backend/traffic.json
```

另开一个 SSH/Xshell 窗口验证：

```sh
cat /tmp/webnet/backend/traffic.json
```

## OpenWrt 上运行 Web

如果 OpenWrt 已经有 Python、Flask 和 flask-cors：

```sh
cd /tmp/webnet/backend
python3 app.py
```

浏览器访问：

```text
http://192.168.31.194:5000
```

此时完整链路为：

```text
OpenWrt eth0
    ↓
webnet-monitor
    ↓
/tmp/webnet/backend/traffic.json
    ↓
Flask
    ↓
Browser
```

## 常见问题

### pip 提示 externally-managed-environment

Ubuntu 24.04 会保护系统 Python 环境，使用虚拟环境即可：

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install flask flask-cors
```

### OpenWrt scp 失败或空间不足

不要上传 `.venv`、`.git`、`docs`、编译产物和临时文件。使用精简 tar 包部署到 `/tmp`。

### `./scripts/firewall.sh: not found`

通常是因为脚本第一行是 `#!/bin/bash`，而 OpenWrt 没有 bash：

```sh
sed -i '1s|.*|#!/bin/sh|' scripts/firewall.sh
```

### `wget -S` 不支持

OpenWrt BusyBox wget 功能较少，使用：

```sh
wget -q -O /dev/null -T 5 http://baidu.com
echo $?
```

### 添加 TCP 80 规则后仍然能 ping

正常。`ping` 使用 ICMP，不是 TCP 80。TCP 80 规则应使用 HTTP 请求验证。

## 发布到 GitHub 前

不要提交虚拟环境、编译产物、临时 JSON、实验截图大文件和 Word 临时文件。

推荐：

```bash
git status
git add README.md .gitignore .gitattributes backend capture frontend scripts openwrt
git commit -m "Add OpenWrt traffic monitor and firewall web app"
git remote add origin <your-github-repo-url>
git push -u origin main
```

如果默认分支是 `master`：

```bash
git push -u origin master
```

## License

For course experiment and learning use.
