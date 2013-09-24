#include <cstdint>
#include <string>
#include <iostream>
#include <cstdio>
#include <sstream>

#include <fstream>

#include <ctime>

std::string read_stream(std::istream &in)
{
  if (in)
  {
    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    return(contents);
  }
  throw(errno);
}

int by_scanf() {
    char data[50];
    unsigned int timestamp;
    char type;
    while (scanf("%u %c %[^\n]", &timestamp, &type, &data) > 0) {}
    //while (scanf("%[^\n]", &data) > 0) {}
    return 0;
}

unsigned unsynced() {
    // we're limited to getline rather than seeking as while we can seek on
    // './foo < bar', we can't on 'cat bar | ./foo' and we're required to unix-pipeline
    // friendly
    std::ios_base::sync_with_stdio (false);
    std::string line;
    line.reserve(32);
    std::istringstream;

    unsigned dependency_var = 0;
    while (getline (std::cin, line)) {
        dependency_var++;
    }
    return dependency_var;
    //std::cout << dependency_var << '\n';
}

int main() {
    clock_t start, end;
    double total_time;

    start=clock();
    //auto data = read_stream(std::cin);
    //auto data = by_scanf();
    auto data = unsynced();
    end=clock();
    total_time = (double)(end-start)/CLOCKS_PER_SEC;
    std::cout << total_time << std::endl;
}
