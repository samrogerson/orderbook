#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cstdint>

#include <ctime>

typedef std::vector<std::string> OrderTokens;

enum class OrderType : char { ADD='A', REDUCE='R' };

int main(int argc, char** argv) {
    clock_t start, end;

    std::string line;
    uint64_t nlines(0), n_add(0), n_reduce(0);
    
    start=clock();
    while(std::getline(std::cin,line)) {
        nlines++;
        std::istringstream iss(line);
        OrderTokens tokens;
        std::copy(std::istream_iterator<std::string>(iss),
            std::istream_iterator<std::string>(),
            std::back_inserter<OrderTokens>(tokens));
            OrderType t = OrderType(tokens[1].c_str()[0]);
            if(t==OrderType::ADD) n_add++;
            if(t==OrderType::REDUCE) n_reduce++;

    }
    end=clock();

    double total_time = (double)(end-start)/CLOCKS_PER_SEC;
    std::cout << "processed " << nlines << " in " << total_time <<
        " seconds" << std::endl;
    std::cout << "ADDs: " << n_add << ", REDUCE: " << n_reduce << std::endl;
    std::cout << "Average process time per record: " << (total_time/double(nlines))*1000 << "ms" <<
        std::endl;
    std::cout << "Speed: " << nlines / total_time << " records per second" << std::endl;
    return 0;
}
