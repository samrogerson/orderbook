#! /bin/bash

bin="pricer.x"
g++ -std=c++11 -march=native -O2 -o ${bin} pricer.cpp

echo "Redirect File"
echo "============="
./${bin} < data/pricer.in 10000
echo
echo "Pipeline"
echo "========"
cat data/pricer.in | ./${bin} 10000
echo

#sizes="1 200 10000"

#for size in ${sizes[@]}; 
#do
    #./${bin} $size < data/pricer.in > test.out.$size
    #diff test.out.$size data/pricer.out.$size
#done
