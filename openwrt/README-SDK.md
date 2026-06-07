# OpenWrt SDK cross compilation

This directory contains an OpenWrt package for building the traffic monitor on
WSL and deploying the resulting binary to OpenWrt.

The tested OpenWrt VM is OpenWrt 24.10.0 x86/64 with interface `eth0` and IP
`192.168.31.194`.

## 1. Prepare SDK on WSL

Download the SDK matching the OpenWrt target:

```bash
cd ~
wget https://downloads.openwrt.org/releases/24.10.0/targets/x86/64/openwrt-sdk-24.10.0-x86-64_gcc-13.3.0_musl.Linux-x86_64.tar.zst
tar --use-compress-program=unzstd -xf openwrt-sdk-24.10.0-x86-64_gcc-13.3.0_musl.Linux-x86_64.tar.zst
cd openwrt-sdk-24.10.0-x86-64_gcc-13.3.0_musl.Linux-x86_64
```

If `unzstd` is missing:

```bash
sudo apt install -y zstd build-essential rsync unzip file
```

## 2. Add this package to the SDK

```bash
cp -r /mnt/d/vibecodeing/webnet/program/openwrt/webnet-monitor package/
```

## 3. Select and build the package

```bash
make menuconfig
```

Select:

```text
Network  --->
  <*> webnet-monitor
```

Then build:

```bash
make package/webnet-monitor/compile V=s
```

The `.ipk` output is under:

```text
bin/packages/x86_64/base/
```

## 4. Copy to OpenWrt

```bash
scp bin/packages/x86_64/base/webnet-monitor_1.0-1_x86_64.ipk root@192.168.31.194:/tmp/
```

## 5. Install and run on OpenWrt

```sh
opkg update
opkg install libpcap
opkg install /tmp/webnet-monitor_1.0-1_x86_64.ipk
mkdir -p /tmp/webnet/backend
webnet-monitor eth0 /tmp/webnet/backend/traffic.json
```

Open another SSH session and check:

```sh
cat /tmp/webnet/backend/traffic.json
```

If data appears in JSON format, the OpenWrt-side traffic monitor is running.
