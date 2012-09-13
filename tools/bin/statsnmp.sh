#! /bin/bash

sleeptime=1
prefix=Tcp
case $# in
    1)
    if [[ $1 =~ [0-9] ]]; then 
        sleeptime=$1; 
    #elif [[ $1 =~ [^0-9] ]]; then
        #prefix=$1;
    else
        prefix=$1;
    fi
    ;;
    2)
    prefix=$1;
    sleeptime=$2;
    ;;
    *)
    echo "run in default mode"
    ;;
esac

file=/proc/net/snmp
filter="^$prefix:"

cat $file | grep "$filter"

while true; do
    sleep $sleeptime
    cat $file | grep "$filter" | tail -n 1
done
