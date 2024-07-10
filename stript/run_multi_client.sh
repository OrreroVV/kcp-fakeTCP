#!/bin/bash

server_ip="8.138.86.207"
server_port=6666
num_clients=26000

# function cleanup {
#     echo "Cleaning up..."
#     sudo pkill kcp_qps
#     exit 0
# }

# trap cleanup SIGINT SIGTERM

cd /home/hzh/workspace/kcp-fakeTCP/bin/

for i in $(seq 20000 $num_clients); do
    sudo ./kcp_qps $i &
done

wait
