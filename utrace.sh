#!/bin/bash

host=$(hostname | awk -F- '{print $NF}')

n=0
mtx=0
mrx=0
mdr=0

while true
do
    day=$(date +"%m-%d")
    rpid=$(pgrep -x roxie)

    lines=$(cat /proc/net/udp | tail -n +2 | grep -v '00000000:00000000')
    while read -r line
    do
        #echo "line = $line"
        read -r sl loc rem st hxx tm retr uid timo inode ref ptr drps < <(echo $line)
        read -r htx hrx < <(echo $hxx | awk -F: '{print $1 " " $2}')
        tx=$((16#$htx))
        rx=$((16#$hrx))
        if [[ $tx -gt 0 ]] || [[ $rx -gt 0 ]] ; then
            ((n++))
            dline=$(date +"%Y-%m-%d %H:%M:%S %N")
            read -r d t ms < <(echo $dline)
            ms=$(($((10#$ms)) / 1000000))
            read -r hlip hlport < <(echo $loc | awk -F: '{print $1 " " $2}')
            lip=$(echo $hlip | awk '{str = sprintf("0x%s", $0); ip = strtonum(str); \
                printf ("%d.%d.%d.%d",rshift(and(ip,0x000000ff),00),
                                      rshift(and(ip,0x0000ff00),08),
                                      rshift(and(ip,0x00ff0000),16),
                                      rshift(and(ip,0xff000000),24))}')
            if [[ $lip = "0.0.0.0" ]] ; then
                lip="."
            fi
            lport=$((16#$hlport))
            p=0
            m=0
            prev=0
            mtx80f=$(bc<<<$mtx*0.80)
            mtx80=${mtx80f%.*}
            mrx80f=$(bc<<<$mrx*0.80)
            mrx80=${mrx80f%.*} 
            if [[ $tx -gt $mtx ]] ; then
                mtx=$tx
                m=1
            fi
            if [[ $tx -gt $mtx80 ]] ; then
                pout=$(printf "%s %s.%03d %14s:%-6d %8d %8d %8d ^--\n" $d $t $ms $lip $lport $tx $rx $drps)
                echo $pout | sudo tee -a /var/lib/HPCCSystems/queries/utrace.${host}.${day}.${rpid} > /dev/null 2>&1
                p=1
            fi
            if [[ $rx -gt $mrx ]] ; then
                mrx=$rx
                m=2
            fi
            if [[ $rx -gt $mrx80 ]] ; then
                pout=$(printf "%s %s.%03d %14s:%-6d %8d %8d %8d -v-\n" $d $t $ms $lip $lport $tx $rx $drps)
                echo $pout | sudo tee -a /var/lib/HPCCSystems/queries/utrace.${host}.${day}.${rpid} > /dev/null 2>&1
                p=2
            fi
            if [[ $drps -gt $mdr ]] ; then
                mdr=$drps
                pout=$(printf "%s %s.%03d %14s:%-6d %8d %8d %8d --x\n" $d $t $ms $lip $lport $tx $rx $drps)
                echo $pout | sudo tee -a /var/lib/HPCCSystems/queries/utrace.${host}.${day}.${rpid} > /dev/null 2>&1
                p=3
            fi
            if [[ $p -gt 0 ]] ; then
                echo "--------"                    | sudo tee -a /var/lib/HPCCSystems/queries/utrace.${host}.${day}.${rpid} > /dev/null 2>&1
                cat /proc/net/sockstat | head -n 3 | sudo tee -a /var/lib/HPCCSystems/queries/utrace.${host}.${day}.${rpid} > /dev/null 2>&1
                echo "--------"                    | sudo tee -a /var/lib/HPCCSystems/queries/utrace.${host}.${day}.${rpid} > /dev/null 2>&1
            fi
        fi
    done <<< "$lines"
    if [[ $n -gt 20 ]] ; then
        n=0
        pout=$(printf "%s %s.%03d %14s:%-6d %8d %8d %8d\n" $d $t $ms $lip $lport $tx $rx $drps)
        echo $pout | sudo tee -a /var/lib/HPCCSystems/queries/utrace.${host}.${day}.${rpid} > /dev/null 2>&1
    fi
    sleep 1
done

