#! /bin/bash

bin="pricer.x"
if [[ ! -f ${bin} ]]
then
    echo -n "Compiling main binary... "
    g++ -Wall -std=c++11 -march=native -O2 -o ${bin} pricer.cpp
    echo "Done."
fi

prof="pricer_prof.x"
if [[ ! -f ${prof} ]]
then
    echo -n "Compiling profiling binary... "
    g++ -pg -std=c++11 -march=native -O2 -o ${prof} pricer.cpp
    echo "Done."
fi

sizes="1 200 10000"
data="data/pricer.in"

echo "---------------------------"
echo "Testing against sample data"
echo "---------------------------"
for size in ${sizes[@]}; 
do
    cat ${data} | ./${bin} $size > test.out.$size
    if [[ -f data/price.out.$size ]];
    then
        diff test.out.$size data/pricer.out.$size
    fi
done
echo "---------------------------"
echo

echo -n "Profiling... "
cat ${data} | ./${prof} 10000 > test.out.10000.prof
gprof ./pricer_prof.x gmon.out > analysis.txt
echo "Done."
