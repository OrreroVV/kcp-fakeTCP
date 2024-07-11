#!/bin/bash

server_ip="8.138.86.207"
server_port=6666
num_clients=500
start=10000


# function cleanup {
#     echo "Cleaning up..."
#     sudo pkill kcp_qps
#     exit 0
# }

# trap cleanup SIGINT SIGTERM

cd /home/hzh/workspace/kcp-fakeTCP/bin/
for ((i=$start; i < $start + $num_clients; i++)); do
    sudo ./kcp_qps $i &  # 在后台执行带参数的命令
done

wait
