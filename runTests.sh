#! /bin/bash

bin="pricer.x"
if [[ ! -f ${bin} ]]
then
    g++ -std=c++11 -march=native -O2 -o ${bin} pricer.cpp
fi

sizes="1 200 10000"

for size in ${sizes[@]}; 
do
    cat data/pricer.in | ./${bin} $size > test.out.$size
    diff test.out.$size data/pricer.out.$size
done
