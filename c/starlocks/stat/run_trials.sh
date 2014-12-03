#!/bin/bash
num=$1
cust=$2
type=$3
quiet=$4 # Quiet flag 

function print_usage {
    echo "Usage: $0 trials customers type (0 = starlocks, 1 = classic) [-q]"
    exit
}

if [[ -z $cust ]]; then
    print_usage
fi

if [[ -z $num ]]; then
    print_usage
fi

if [[ $type -eq 1 ]]
then
    if [[ "$quiet" != "-q" ]]; then
        echo "Running $num trials in classic mode (1 queue, 2 baristas) with $cust customers."
    fi
else 
    if [[ "$quiet" != "-q" ]]; then
    echo "Running $num trials in starlocks mode (3 queues, 1 selfserve, 1 barista, 1 cashier) with $cust customers." 
    fi
fi

fmt="Trial\tCustomers\tTime\tAvg_Simple\tAvg_Complex\tProfit\n"
TIMEFORMAT="%U"
for ((i=0;i<$num;i++))
do
    if [ $type -eq 1 ]; 
    then
        time[$i]=$( { time ./starlocks $cust -b 2 -q > tmp; } 2>&1 )
    else
        time[$i]=$( { time ./starlocks $cust -b 1 -s 1 -c 1-q > tmp; } 2>&1 )
    fi
    simple[$i]=`grep Simple tmp | awk '{print $4}'`
    complex[$i]=`grep Complex tmp | awk '{print $3}'`
    profit[$i]=`grep Profit tmp | awk '{print $3}'`
    fmt+="$i\t$cust\t${time[$i]}\t${simple[$i]}\t${complex[$i]}\t${profit[$i]}\n"
done

echo -e $fmt | column -t
