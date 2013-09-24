#! /bin/bash

g++ -O2 -std=c++11 -o io.x testing/io.cpp 
./io.x < data/pricer.in
cat data/pricer.in | ./io.x

#sizes="1 200 10000"

#for size in ${sizes[@]}; 
#do
    #./a.out $size < data/pricer.in > test.out.$size
    #diff test.out.$size data/pricer.out.$size
#done
