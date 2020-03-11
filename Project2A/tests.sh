#!/bin/bash


threads=(2 4 8 12) 
iterations=(10 20 40 80 100 1000 10000 100000)
for i in "${threads[@]}"
do
    for j in "${iterations[@]}"
    do
        ./lab2_add --threads=$i --iterations=$j --yield >> lab2_add.csv
    done
done

# /1

# 2

threads=(2 8)
iterations=(100 1000 10000 100000) 
for i in "${threads[@]}"
do
    for j in "${iterations[@]}"
    do
        ./lab2_add --threads=$i --iterations=$j --yield >> lab2_add.csv
        ./lab2_add --threads=$i --iterations=$j >> lab2_add.csv
    done
done

# /2

# 3

threads=(1 2 4 8 12 16) 
iterations=(1 10 100 1000 10000 100000)
for j in "${iterations[@]}"
do
    ./lab2_add --iterations=$j >> lab2_add.csv
done

# /3

# 4

syncs=(m s c)
iterations=(10 100 1000 10000)
for i in "${threads[@]}"
do 
    for j in "${iterations[@]}"
    do
        ./lab2_add --iterations=$j --threads=$i --yield >> lab2_add.csv
        for k in "${syncs[@]}"
        do
            ./lab2_add --iterations=$j --threads=$i --sync=$k --yield >> lab2_add.csv
        done
    done
done

# /4

# 5

for i in "${threads[@]}"
do 
    ./lab2_add --iterations=10000 --threads=$i >> lab2_add.csv
    for k in "${syncs[@]}"
    do
        ./lab2_add --iterations=10000 --threads=$i --sync=$k >> lab2_add.csv
    done
done

# /5




# 1

iterations=(1 10 100 1000 10000 20000)
for j in "${iterations[@]:1}"
do
    ./lab2_list --iterations=$j >> lab2_list.csv
done


yields=(i d il dl)
iterations=(1 2 4 8 16 32)
threads=(1 2 4 8 12)

for i in "${threads[@]:1}"
do
    for j in "${iterations[@]}"
    do
        ./lab2_list --threads=$i --iterations=$j >> lab2_list.csv
        for k in "${yields[@]}"
        do
            ./lab2_list --threads=$i --iterations=$j --yield=$k >> lab2_list.csv
        done
    done
done

syncs=(m s)

for k in "${yields[@]}"
do
    for l in "${syncs[@]}"
    do
        ./lab2_list --threads=12 --iterations=32 --yield=$k --sync=$l >> lab2_list.csv
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