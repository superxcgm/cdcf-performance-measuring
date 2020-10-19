#!/usr/bin/expect -f

set timeout 600
spawn ./cdcf_performance_measuring
send "enqueueing 10000000\r"
send "dequeueing 10000000\r"
send "initiation 100000\r"
send "single-producer-sending 6000000\r"
send "multi-producer-sending 6000000\r"
send "max-throughput 12000000\r"
send "ping-latency 1500000\r"
send "ping-throughput-10k 2000000\r"

send "enqueueing 10000000\r"
send "dequeueing 10000000\r"
send "initiation 100000\r"
send "single-producer-sending 6000000\r"
send "multi-producer-sending 6000000\r"
send "max-throughput 12000000\r"
send "ping-latency 1500000\r"
send "ping-throughput-10k 2000000\r"

send "enqueueing 10000000\r"
send "dequeueing 10000000\r"
send "initiation 100000\r"
send "single-producer-sending 6000000\r"
send "multi-producer-sending 6000000\r"
send "max-throughput 12000000\r"
send "ping-latency 1500000\r"
send "ping-throughput-10k 2000000\r"
send "q\r"
expect "exit"
exit

