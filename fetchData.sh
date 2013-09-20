#! /bin/bash

declare -a data_urls=(
    "http://www.rgmadvisors.com/problems/orderbook/pricer.in.gz"
    "http://www.rgmadvisors.com/problems/orderbook/pricer.out.1.gz"
    "http://www.rgmadvisors.com/problems/orderbook/pricer.out.200.gz"
    "http://www.rgmadvisors.com/problems/orderbook/pricer.out.10000.gz"
    )

for url in ${data_urls[@]}
do
    gz_filename=data/${url##*/}
    data_filename=${gz_filename%.gz}
    if [[ ! -f $data_filename ]]
    then
        if [[ ! -f $gz_filename ]];
        then
            wget $url -O $gz_filename
        fi
        gzip -d $gz_filename
    else
        echo $data_filename already exists
    fi
done
