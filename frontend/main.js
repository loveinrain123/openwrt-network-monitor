let trafficChart = null;

// 从后端读取 traffic.json 对应的流量数据，并刷新表格和图表。
async function loadTraffic(){

    const res = await fetch("/api/traffic");

    const data = await res.json();

    const tbody = document.getElementById("trafficTable");

    tbody.innerHTML = "";

    data.forEach(item => {

        const tr = document.createElement("tr");

        tr.innerHTML = `
            <td>${item.ip}</td>
            <td>${item.tx}</td>
            <td>${item.rx}</td>
            <td>${item.packets}</td>
            <td>${item.peak}</td>
        `;

        tbody.appendChild(tr);

    });

    updateChart(data);
}

// 使用 Chart.js 绘制 TX/RX 柱状图；首次创建，之后只更新数据。
function updateChart(data){

    const labels = data.map(item => item.ip);

    const txData = data.map(item => item.tx);

    const rxData = data.map(item => item.rx);

    const ctx = document.getElementById("trafficChart");

    if(!trafficChart){

        trafficChart = new Chart(ctx, {

            type:"bar",

            data:{
                labels:labels,

                datasets:[
                    {
                        label:"发送流量 TX(Bytes)",
                        data:txData
                    },
                    {
                        label:"接收流量 RX(Bytes)",
                        data:rxData
                    }
                ]
            },

            options:{
                responsive:true,
                maintainAspectRatio:false,

                plugins:{
                    legend:{
                        labels:{
                            color:"white",
                            font:{
                                size:18
                            }
                        }
                    }
                },

                scales:{
                    x:{
                        ticks:{
                            color:"white",
                            font:{
                                size:16
                            }
                        },

                        grid:{
                            color:"#1e293b"
                        }
                    },

                    y:{
                        ticks:{
                            color:"white",
                            font:{
                                size:16
                            }
                        },

                        grid:{
                            color:"#1e293b"
                        }
                    }
                }
            }

        });

    }else{

        trafficChart.data.labels = labels;

        trafficChart.data.datasets[0].data = txData;

        trafficChart.data.datasets[1].data = rxData;

        trafficChart.update();

    }
}

// 从后端读取当前 iptables 规则。
async function loadRules(){

    const res = await fetch("/api/firewall/list");

    const data = await res.json();

    document.getElementById("rules").textContent =
        data.stdout || data.stderr;

}

// 添加端口 DROP 规则。chain 为 INPUT 或 OUTPUT。
async function addRule(){

    const proto = document.getElementById("proto").value;

    const chain = document.getElementById("chain").value;

    const port = document.getElementById("port").value;

    const res = await fetch("/api/firewall/add",{

        method:"POST",

        headers:{
            "Content-Type":"application/json"
        },

        body:JSON.stringify({
            proto:proto,
            chain:chain,
            port:port
        })

    });

    const data = await res.json();

    alert(data.stdout || data.stderr);

    loadRules();
}

// 封禁指定目标 IP。
async function blockIP(){

    const ip = document.getElementById("ip").value;

    const res = await fetch("/api/firewall/blockip",{

        method:"POST",

        headers:{
            "Content-Type":"application/json"
        },

        body:JSON.stringify({
            ip:ip
        })

    });

    const data = await res.json();

    alert(data.stdout || data.stderr);

    loadRules();
}

// 清空当前 iptables 规则。
async function clearRules(){

    const res = await fetch("/api/firewall/clear",{
        method:"POST"
    });

    const data = await res.json();

    alert(data.stdout || data.stderr);

    loadRules();
}

// 页面加载后先请求一次数据。
loadTraffic();
loadRules();

// 自动刷新：流量变化更快，防火墙规则变化较少。
setInterval(loadTraffic,1000);
setInterval(loadRules,3000);
