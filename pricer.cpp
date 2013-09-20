#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <map>
#include <unordered_map>

#include <ctime>

struct Order;

typedef std::vector<std::string> OrderTokens;
typedef std::map<std::string, Order> OrderBook;
typedef std::pair<int, int> BookSummary;

enum class MessageType : char { ADD='A', REDUCE='R' };
enum class OrderType : char { BUY='B', SELL='S', NA };

struct Message {
        MessageType mtype;
        OrderType otype;
        int size;
        int timestamp;
        double price;
        std::string ID;

        Message(OrderTokens &tokens) : mtype(MessageType(tokens[1].c_str()[0])), ID(tokens[2]),
        timestamp(atoi(tokens[0].c_str())) {
            int size_pos = (mtype == MessageType::ADD) ? 5 : 3;
            size = atoi(tokens[size_pos].c_str());
            // this is being checked twice because of above: combine
            if(mtype == MessageType::ADD) {
                otype = OrderType(tokens[3].c_str()[0]);
                price = atof(tokens[4].c_str());
            } else {
                otype = OrderType::NA;
            }
        }
        
        void print() {
            std::cout << timestamp << " " << char(mtype) << " " << ID << " ";
            if(otype != OrderType::NA) {
                std::cout << char(otype) << " " << price << " ";
            }
            std::cout << size << std::endl;
        }
};

struct Order {
        OrderType type;
        int size;
        double price;

        Order(){}
        Order(OrderType& t, int s, double& p) : type(t), size(s), price(p) {}
        Order(const Message& m) : type(m.otype), size(m.size), price(m.price) {}

        Order& operator-=(const int& deduction) {
            size -= deduction;
        }
        Order& operator+=(const int& addition) {
            size += addition;
        }

        void reduce(const Message& m) {
            size -= m.size;
        }

};

bool process_message(const Message &m, OrderBook &b) {
    if(m.mtype==MessageType::ADD) {
        if(b.find(m.ID) == b.end()) {
            b[m.ID] = Order(m);
        } else {
            std::cerr << "[ERR] received ADD order for existing order ID=" << m.ID << std::endl;
        }
    }
    if(m.mtype==MessageType::REDUCE) {
        OrderBook::iterator order = b.find(m.ID);
        if(order != b.end()) {
            order->second.reduce(m);
            if(order->second.size<=0) {
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
        m.print();
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
