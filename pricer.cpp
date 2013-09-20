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

#include <ctime>

struct Order;

typedef std::vector<std::string> OrderTokens;
typedef std::map<std::string, Order*> OrderLookup;
typedef std::map<std::string, Order> Book;

enum class MessageType : char { ADD='A', REDUCE='R' };
enum class OrderType : char { BID='B', ASK='S', NA };

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

        int reduce(const Message& m) {
            int reduced_by = m.size;
            if(m.size > size) reduced_by = size;
            size -= m.size;
            return reduced_by;
        }

};

struct OrderBook {
    Book orders;
    OrderLookup asks, bids;
    std::map<OrderType, OrderLookup*> lookup_reference;
    int total_asks, total_bids;    

    OrderBook() {
        total_asks = 0;
        total_bids = 0;

        lookup_reference[OrderType::ASK] = &asks;
        lookup_reference[OrderType::BID] = &bids;
    }
        

    bool order_exists(const std::string& ID) {
        return orders.find(ID)!=orders.end();
    }
        
    int add_order(const Message &m) {
        if(!order_exists(m.ID)) {
            Order o(m);
            orders[m.ID] = o;
            OrderLookup * l = lookup_reference[m.otype];
            (*l)[m.ID] = &(orders[m.ID]);
            if(o.type == OrderType::ASK) total_asks += o.size;
            else if(o.type == OrderType::BID) total_bids += o.size;
        } else {
            std::cerr << "[ERR] received ADD order for existing order ID=" << m.ID << std::endl;
            return -1;
        }
        return 0;
    }

    int reduce_order(const Message &m) {
        if(order_exists(m.ID)) {
            Book::iterator id_order = orders.find(m.ID);
            int deduction = id_order->second.reduce(m);
            if(id_order->second.type == OrderType::ASK) total_asks -= deduction;
            else if(id_order->second.type == OrderType::BID) total_bids -= deduction;
            if(id_order->second.size<=0) {
                orders.erase(id_order->first);
                lookup_reference[id_order->second.type]->erase(id_order->first);
            }
        } else {
            std::cerr << "[ERR] received REDUCE order for non-existant order ID=" << m.ID << std::endl;
        }
        return 0;
    }
    
    bool update(const Message &m) {
        if(m.mtype==MessageType::ADD) {
            add_order(m);
        }
        if(m.mtype==MessageType::REDUCE) {
            reduce_order(m);
        }
        //std::cout << "asks: " << total_asks << ", bids: " << total_bids << std::endl;
    }
};

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
        //m.print();
        book.update(m);
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
