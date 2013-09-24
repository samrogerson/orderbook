#! /bin/bash

g++ -std=c++11 -march=native -O2 -o spirit.x testing/spirit_test.cpp

echo "Redirect File"
echo "============="
./spirit.x < data/pricer.in
echo
echo "Pipeline"
echo "========"
cat data/pricer.in | ./spirit.x

#bin="spirit.x"
#sizes="1 200 10000"

#for size in ${sizes[@]}; 
#do
    #./${bin} $size < data/pricer.in > test.out.$size
    #diff test.out.$size data/pricer.out.$size
#done
