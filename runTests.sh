#! /bin/bash

g++ -O2 -std=c++11 pricer.cpp

sizes="1 200 10000"

for size in ${sizes[@]}; 
do
    ./a.out $size < data/pricer.in > test.out.$size
    diff test.out.$size data/pricer.out.$size
done
