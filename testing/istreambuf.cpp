#include <iostream>
#include <string>
#include <sstream>

int main() {
    using std::string;
    using std::stringstream;
    using std::cin;
    using std::istreambuf_iterator;
    using std::cout;
    using std::endl;
    //string in;
    //in.reserve(32*2e6);
    //in.assign(istreambuf_iterator<char>(cin), istreambuf_iterator<char>());
    string str(static_cast<stringstream const&>(stringstream() << cin.rdbuf()).str());
    cout << str << endl;
    //std::string str(static_cast<std::stringstream const&>(std::stringstream() << std::cin.rdbuf()).str());
    //std::size_t found = 0;
    //std::size_t max = str.size();
    //std::string::const_iterator iter = str.begin();
}


