#!/bin/bash

# NAME: Derek Vance
# EMAIL: dvance@g.ucla.edu
# UID: 604970765

# 1

threads=(1 2 4 8 12 16 24) 

for i in "${threads[@]}"
do
    ./lab2_list --threads=$i --iterations=1000 --sync=m >> lab2b_list.csv
    ./lab2_list --threads=$i --iterations=1000 --sync=s >> lab2b_list.csv
done

# /1

# 2



# /2

# 3

for i in "${threads[@]}"
do
    iterations=(1 2 4 8 16)
    for j in "${iterations[@]}"
    do
        ./lab2_list --threads=$i --iterations=$j --lists=4 --yield=id >> lab2b_list.csv
    done
    iterations=(10 20 40 80)
    for j in "${iterations[@]}"
    do
        ./lab2_list --threads=$i --iterations=$j --lists=4 --yield=id --sync=m >> lab2b_list.csv
        ./lab2_list --threads=$i --iterations=$j --lists=4 --yield=id --sync=s >> lab2b_list.csv
    done
done

# /3

# 4/5

threads=(1 2 4 8 12)
lists=(4 8 16)
for i in "${threads[@]}"
do
    for k in "${lists[@]}"
    do
        ./lab2_list --threads=$i --iterations=1000 --lists=$k --sync=m >> lab2b_list.csv
        ./lab2_list --threads=$i --iterations=1000 --lists=$k --sync=s >> lab2b_list.csv
    done
done
# /4/5