#include <iostream>
#include <string>

int main() {
    std::string in;
    in.reserve(32*2e6);
    in.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
    std::cout << in << std::endl;
}

