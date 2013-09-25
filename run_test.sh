#! /bin/bash

bin="pricer.x"
if [[ ! -f ${bin} ]]
then
    echo -n "Compiling... "
    g++ -std=c++11 -march=native -O2 -o ${bin} pricer.cpp
    echo "Done."
fi

prof="pricer_prof.x"
if [[ ! -f ${prof} ]]
then
    g++ -pg -std=c++11 -march=native -O2 -o ${prof} pricer.cpp
fi

sizes="1 200 10000"
data="data/pricer.in"

echo "---------------------------"
echo "Testing against sample data"
echo "---------------------------"
for size in ${sizes[@]}; 
do
    cat ${data} | ./${bin} $size > test.out.$size
    diff test.out.$size data/pricer.out.$size
done
echo "---------------------------"
echo

echo -n "Profiling... "
cat ${data} | ./${prof} 10000 > test.out.10000.prof
gprof ./pricer_prof.x gmon.out > analysis.txt
echo "Done."
