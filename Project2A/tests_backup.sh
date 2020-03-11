#!/bin/bash

threads=(1 2 4 8 12 16) 
iterations=(1 10 100 1000 10000 100000) 
for i in "${threads[@]}"
do
    for j in "${iterations[@]:2}"
    do
        ./lab2_add --threads=$i --iterations=$j >> lab2_add.csv
    done
done

iterations=(10 20 40 80 100 1000 10000 100000)
for i in "${threads[@]:1}"
do
    for j in "${iterations[@]}"
    do
        ./lab2_add --threads=$i --iterations=$j --yield >> lab2_add.csv
    done
done

for i in "${threads[@]::4}"
do
    ./lab2_add --threads=$i --iterations=1000000 >> lab2_add.csv
done

iterations=(1 10 100 1000 10000 100000) 
syncs=(m s c)
ifyield=(0 1)
for i in "${threads[@]::4}"
do 
    for k in "${syncs[@]}"
    do
        for l in "${ifyield[@]}"
        do
            if [ $l -gt 0 ]
            then
                ./lab2_add --threads=$i --iterations=10000 --sync=$k --yield >> lab2_add.csv
            else
                ./lab2_add --threads=$i --iterations=10000 --sync=$k >> lab2_add.csv
            fi
        done
    done
done




echo 'starting list'



iterations=(1 10 100 1000 10000 20000)
for j in "${iterations[@]:1}"
do
    ./lab2_list --iterations=$j >> lab2_list.csv
done

for i in "${threads[@]:1}"
do
    for j in "${iterations[@]::4}"
    do
        ./lab2_list --threads=$i --iterations=$j >> lab2_list.csv
    done
done

yields=(i d il dl)
iterations=(1 2 4 8 16 32)
for i in "${threads[@]:1}"
do
    for j in "${iterations[@]}"
    do
        for k in "${yields[@]}"
        do
            ./lab2_list --threads=$i --iterations=$j --yield=$k >> lab2_list.csv
        done
    done
done

syncs=(m s)
for i in "${threads[@]::4}"
do
    for j in "${iterations[@]}"
    do
        for k in "${yields[@]}"
        do
            for l in "${syncs[@]}"
            do
                ./lab2_list --threads=$i --iterations=$j --yield=$k --sync=$l >> lab2_list.csv
            done
        done
    done
done

threads=(1 2 4 8 12 16 24)
for i in "${threads[@]}"
do
    for l in "${syncs[@]}"
    do
        ./lab2_list --threads=$i --iterations=1000 --sync=$l >> lab2_list.csv
    done
done