from flask import Flask, jsonify, request, send_from_directory
from flask_cors import CORS
import json
import os
import subprocess
import re
import shutil

app = Flask(__name__)
CORS(app)

# 根据当前文件位置计算项目路径，避免从不同目录启动时找不到文件。
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(BASE_DIR)

TRAFFIC_FILE = os.path.join(BASE_DIR, "traffic.json")
FRONTEND_DIR = os.path.join(PROJECT_DIR, "frontend")
SCRIPT_PATH = os.path.join(PROJECT_DIR, "scripts", "firewall.sh")


def run_firewall_script(*args):
    """Run firewall.sh and return stdout/stderr/code to the frontend."""
    cmd = [SCRIPT_PATH, *args]

    # WSL 普通用户需要 sudo；OpenWrt 通常直接以 root 运行，没有 sudo。
    if os.name != "nt" and os.geteuid() != 0 and shutil.which("sudo"):
        cmd.insert(0, "sudo")

    return subprocess.run(
        cmd,
        capture_output=True,
        text=True
    )


@app.route("/")
def index():
    # 返回前端首页。
    return send_from_directory(FRONTEND_DIR, "index.html")


@app.route("/<path:path>")
def static_files(path):
    # 返回 CSS、JS 等静态资源。
    return send_from_directory(FRONTEND_DIR, path)


@app.route("/api/traffic")
def get_traffic():
    # traffic.json 由 capture/monitor.c 持续写入。
    if not os.path.exists(TRAFFIC_FILE):
        return jsonify([])

    try:
        with open(TRAFFIC_FILE, "r", encoding="utf-8") as f:
            data = json.load(f)
        return jsonify(data)
    except Exception as e:
        return jsonify({"error": str(e)}), 500


@app.route("/api/firewall/list")
def firewall_list():
    # 查看当前 iptables 规则。
    result = run_firewall_script("list")

    return jsonify({
        "stdout": result.stdout,
        "stderr": result.stderr,
        "code": result.returncode
    })


@app.route("/api/firewall/add", methods=["POST"])
def firewall_add():
    data = request.get_json()

    proto = data.get("proto", "tcp")
    port = data.get("port", "")
    chain = data.get("chain", "INPUT").upper()

    # 参数白名单校验，避免把不可信输入直接交给系统命令。
    if proto not in ["tcp", "udp"]:
        return jsonify({"error": "协议只能是 tcp 或 udp"}), 400

    if chain not in ["INPUT", "OUTPUT"]:
        return jsonify({"error": "方向只能是 INPUT 或 OUTPUT"}), 400

    if not port.isdigit():
        return jsonify({"error": "端口必须是数字"}), 400

    result = run_firewall_script("add", proto, port, chain)

    return jsonify({
        "stdout": result.stdout,
        "stderr": result.stderr,
        "code": result.returncode
    })


@app.route("/api/firewall/blockip", methods=["POST"])
def firewall_blockip():
    data = request.get_json()
    ip = data.get("ip", "")

    # 基础 IPv4 格式校验。
    ip_pattern = r"^(\d{1,3}\.){3}\d{1,3}$"

    if not re.match(ip_pattern, ip):
        return jsonify({"error": "IP 地址格式不合法"}), 400

    result = run_firewall_script("blockip", ip)

    return jsonify({
        "stdout": result.stdout,
        "stderr": result.stderr,
        "code": result.returncode
    })


@app.route("/api/firewall/clear", methods=["POST"])
def firewall_clear():
    # 清空当前 iptables 规则，实验演示后用于恢复环境。
    result = run_firewall_script("clear")

    return jsonify({
        "stdout": result.stdout,
        "stderr": result.stderr,
        "code": result.returncode
    })


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
