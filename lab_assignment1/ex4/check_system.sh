#!/bin/bash

####################
# Lab 1 Exercise 4
# Name:
# Student No:
# Lab Group: 
####################

# fill the below up
hostname=$(uname -n)
kernel_version=$(uname -v)
process_cnt=$(ps -A | wc -l)
user_process_cnt=$(ps -x | wc -l)
mem_usage=$(free -t | awk  'NR == 2{print $3/$2}')
swap_usage=$(free -t | awk 'NR == 3{print $3/$2}')

echo "Hostname: $hostname"
echo "Linux Kernel Version: $kernel_version"
echo "Total Processes: $process_cnt"
echo "User Processes: $user_process_cnt"
echo "Memory Used (%): $mem_usage"
echo "Swap Used (%): $swap_usage"
