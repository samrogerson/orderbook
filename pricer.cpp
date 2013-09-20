#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <unordered_map>

#include <ctime>

typedef std::vector<std::string> OrderTokens;
typedef std::map<std::string, int> OrderBook;

enum class MessageType : char { ADD='A', REDUCE='R' };
enum class OrderType : char { BUY='B', SELL='S' };

class Message {
    friend bool process_message(const Message&, OrderBook&);
    private:
        MessageType mtype;
        OrderType otype;
        int size;
        int timestamp;
        std::string ID;
    public:

        Message(OrderTokens &tokens) : mtype(MessageType(tokens[1].c_str()[0])), ID(tokens[2]),
        timestamp(atoi(tokens[0].c_str())) {
            int size_pos = (mtype == MessageType::ADD) ? 5 : 3;
            size = atoi(tokens[size_pos].c_str());
            // this is being checked twice because of above: combine
            if(mtype == MessageType::ADD) {
                otype = OrderType(tokens[3].c_str()[0]);
            }
        }
};

bool process_message(const Message &m, OrderBook &b) {
    if(m.mtype==MessageType::ADD) {
        if(b.find(m.ID) == b.end()) {
            b[m.ID] = m.size;
        } else {
            std::cerr << "[ERR] received ADD order for existing order ID=" << m.ID << std::endl;
        }
    }
    if(m.mtype==MessageType::REDUCE) {
        OrderBook::iterator order = b.find(m.ID);
        if(order != b.end()) {
            order->second -= m.size;
            if(order->second<=0) {
                b.erase(order);
            }
        } else {
            std::cerr << "[ERR] received REDUCE order for non-existant order ID=" << m.ID << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "This program takes 1 argument" << std::endl;
        return -1;
    }

    int target_size = atoi(argv[1]);

    OrderBook book;
    std::string line;
    uint64_t nlines(0);
    
    clock_t start, end;
    start=clock();
    while(std::getline(std::cin,line)) {
        nlines++;
        std::istringstream iss(line);

        OrderTokens tokens;
        std::copy(std::istream_iterator<std::string>(iss),
            std::istream_iterator<std::string>(),
            std::back_inserter<OrderTokens>(tokens));

        Message m(tokens);
        process_message(m,book);
    }
    end=clock();

    double total_time = (double)(end-start)/CLOCKS_PER_SEC;
    std::cout << "processed " << nlines << " in " << total_time <<
        " seconds" << std::endl;
    std::cout << "Average process time per record: " << (total_time/double(nlines))*1000 << "ms" <<
        std::endl;
    std::cout << "Speed: " << nlines / total_time << " records per second" << std::endl;
    return 0;
}
