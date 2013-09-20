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
typedef std::pair<std::string, Order*> LookupEntry;
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

        Message(OrderTokens &tokens) : mtype(MessageType(tokens[1].c_str()[0])),
        ID(tokens[2]), timestamp(atoi(tokens[0].c_str())) {
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

struct LookupCompare {
        bool operator()(const LookupEntry &lhs, const LookupEntry &rhs) {
            return lhs.second->price < rhs.second->price;
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
            std::cerr << "[ERR] received ADD order for existing order ID=" <<
                m.ID << std::endl;
            return -1;
        }
        return 0;
    }

    int reduce_order(const Message &m) {
        if(order_exists(m.ID)) {
            Book::iterator id_order = orders.find(m.ID);
            int deduction = id_order->second.reduce(m);
            if(id_order->second.type == OrderType::ASK) {
                total_asks -= deduction;
            } else if(id_order->second.type == OrderType::BID) {
                total_bids -= deduction;
            }
            if(id_order->second.size<=0) {
                orders.erase(id_order->first);
                lookup_reference[id_order->second.type]->erase(id_order->first);
            }
        } else {
            std::cerr <<
                "[ERR] received REDUCE order for non-existant order ID=" <<
                m.ID << std::endl;
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
    }
};

class TransactionManager {
    uint64_t nmessages;
    int target_size;
    std::istream *stream;
    OrderBook book;

    bool purchase_state, sell_state;
    double earnings, expenditure;

    public:
        TransactionManager(int target, std::istream &input_stream) :
            nmessages(0), target_size(target), stream(&input_stream),
            purchase_state(false), sell_state(false) { }

        double sell(int time) {
            std::vector<LookupEntry> bid_portfolio(book.bids.begin(),
                    book.bids.end());
            std::sort(bid_portfolio.begin(),bid_portfolio.end(),
                    LookupCompare());
            return -1.;
        }

        double purchase(int time) {
            std::vector<LookupEntry> ask_portfolio(book.asks.begin(),
                    book.asks.end());
            std::sort(ask_portfolio.begin(),ask_portfolio.end(),
                    LookupCompare());
            return 1.;
        }
        
        void update_states(const Message &m) {
            if(purchase_state && book.total_asks < target_size) {
                std::cout << m.timestamp << " B NA" <<  std::endl;
                purchase_state = false;
            } 
            if(sell_state && book.total_bids < target_size) {
                std::cout << m.timestamp << " S NA" <<  std::endl;
                sell_state = false;
            }
            if(book.total_asks >= target_size) {
                purchase_state = true;
                double cost = purchase(m.timestamp);
                if(cost < expenditure) {
                    expenditure = cost;
                    std::cout << m.timestamp << " B " << expenditure <<
                        std::endl;
                }
            }
            if(book.total_bids >= target_size) {
                sell_state = true;
                double income = sell(m.timestamp);
                if(income > earnings) {
                    earnings = income;
                    std::cout << m.timestamp << " S " << earnings <<  std::endl;
                }
            }
                    
        }


        int process() {
            std::string line;
            while(std::getline(*stream,line)) {
                nmessages++;
                std::istringstream iss(line);

                OrderTokens tokens;
                std::copy(std::istream_iterator<std::string>(iss),
                    std::istream_iterator<std::string>(),
                    std::back_inserter<OrderTokens>(tokens));

                Message m(tokens);
                book.update(m);
                update_states(m);
            }
            return nmessages;
        }

};

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "This program takes 1 argument" << std::endl;
        return -1;
    }

    int target_size = atoi(argv[1]);

    TransactionManager t(target_size,std::cin);
    
    clock_t start, end;
    start=clock();
    uint64_t nmessages = t.process();
    end=clock();

    double total_time = (double)(end-start)/CLOCKS_PER_SEC;
    std::cout << "processed " << nmessages << " in " << total_time <<
        " seconds" << std::endl;
    std::cout << "Average process time per record: " <<
        (total_time/double(nmessages))*1000 << "ms" << std::endl;
    std::cout << "Speed: " << nmessages / total_time <<
        " records per second" << std::endl;
    return 0;
}
