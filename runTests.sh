#! /bin/bash

bin="pricer.x"
#g++ -std=c++11 -march=native -O2 -o ${bin} pricer.cpp

#sizes="1 200 10000"
sizes="200"

for size in ${sizes[@]}; 
do
    #cat data/pricer.in | ./${bin} $size > test.out.$size
    head -n4384 data/pricer.in | ./${bin} $size > test.out.$size
    diff test.out.$size data/pricer.out.$size
done
