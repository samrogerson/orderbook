#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <utility>
#include <map>

#include <ctime>

struct Order;

typedef std::vector<std::string> OrderTokens;
typedef std::map<std::string, Order*> OrderLookup;
typedef std::pair<std::string, Order*> LookupEntry;
typedef std::map<std::string, Order> Book;
typedef std::pair<std::string, int> Transaction;

enum class MessageType : char { ADD='A', REDUCE='R' };
enum class OrderType : char { BID='B', ASK='S', NA };

OrderTokens tokenize(std::string line) {
    std::istringstream iss(line);
    OrderTokens tokens;
    std::copy(std::istream_iterator<std::string>(iss),
        std::istream_iterator<std::string>(),
        std::back_inserter<OrderTokens>(tokens));
    return tokens;
}

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
        
        void print() const {
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

        void print() const {
            std::cout << "[" << char(type) <<  " " << size << " " << price << "]" << std::endl;
        }
};

struct LookupCompare {
        // sort iterable of pairs of <ID, Order> by increasing price.
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

    void print() const {
        for(auto& o : orders) {
            o.second.print();
        }
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
                lookup_reference[id_order->second.type]->erase(id_order->first);
                orders.erase(id_order->first);
            }
        } else {
            std::cerr <<
                "[ERR] received REDUCE order for non-existant order ID=" <<
                m.ID << std::endl;
        }
        return 0;
    }
    
    bool update(const std::vector<Message> &messages) {
        for(auto& m: messages) {
            if(m.mtype==MessageType::ADD) {
                add_order(m);
            }
            if(m.mtype==MessageType::REDUCE) {
                reduce_order(m);
            }
        }
    }
};

class TransactionManager {
    uint64_t nmessages;
    int target_size;
    std::istream *stream;
    OrderBook book;

    double earnings, expenditure;
    bool have_bought, have_sold;

    public:
        TransactionManager(int target, std::istream &input_stream) :
            nmessages(0), target_size(target), stream(&input_stream),
            earnings(0), expenditure(-1), have_bought(false), have_sold(false)
        {}

        double make_sale(int time) {
            std::vector<LookupEntry> bid_portfolio(book.bids.begin(),
                    book.bids.end());
            std::sort(bid_portfolio.begin(),bid_portfolio.end(),
                    LookupCompare());
            int total_sold = 0;
            double total_takings = 0;
            std::vector<LookupEntry>::iterator p = bid_portfolio.end();
            std::vector<Transaction> transactions;
            while(total_sold < target_size) {
                p--;
                int available = p->second->size;
                double price = p->second->price;
                int selling = std::min(available, target_size-total_sold);
                total_sold += selling;
                transactions.push_back({p->first, selling});
                total_takings += selling * price;
            }
            double diff = fabs(total_takings - earnings);
            earnings = total_takings;
            have_sold = true;
            if(diff >= 0.005) {
                std::cout << time << " S " << earnings <<  std::endl;
            }
            return total_takings;
        }

        double make_purchase(int time) {
            std::vector<LookupEntry> ask_portfolio(book.asks.begin(),
                    book.asks.end());
            std::sort(ask_portfolio.begin(),ask_portfolio.end(),
                    LookupCompare());
            int total_bought = 0;
            double total_price = 0;
            std::vector<LookupEntry>::iterator p = ask_portfolio.begin();
            std::vector<Transaction> transactions;
            while(total_bought < target_size) {
                int available = p->second->size;
                double price = p->second->price;
                int buying = std::min(available, target_size-total_bought);
                total_bought += buying;
                transactions.push_back({p->first, buying});
                total_price += buying * price;
                p++;
            }
            double diff = fabs(total_price - expenditure);
            expenditure = total_price;
            have_bought = true;
            if(diff >= 0.005) {
                std::cout << time << " B " << expenditure << std::endl;
            }
            return total_price;
        }

        void update_states(int time) {
            //if(time==31171613 ) {
                //std::cout << "--" << std::endl;
                //book.print();
            //}
            if(have_bought && book.total_asks < target_size) {
                std::cout << time << " B NA" <<  std::endl;
                expenditure = -1;
                have_bought = false;
            } 
            if(have_sold && book.total_bids < target_size) {
                std::cout << time << " S NA" <<  std::endl;
                earnings = 0;
                have_sold = false;
            }
            if(book.total_asks >= target_size) {
                make_purchase(time);
            }
            if(book.total_bids >= target_size) {
                make_sale(time);
            }
        }

        //int process() {
            //std::string line;
            //std::vector<Message> messages;
            //std::getline(*stream,line);
            //nmessages++;
            //OrderTokens tokens = tokenize(line);
            //Message m(tokens);
            //int current_time = m.timestamp;
            //std::string currID = m.ID;
            //messages.push_back(m);

            //while(std::getline(*stream,line)) {
                //nmessages++;
                //tokens = tokenize(line);
                //m = Message(tokens);
                //if(m.timestamp == current_time && m.ID == currID) {
                    //messages.push_back(m);
                //} else {
                    //book.update(messages);
                    //update_states(current_time);
                    //current_time = m.timestamp;
                    //currID = m.ID;
                    //messages.clear();
                    //messages.push_back(m);
                //}
            //}
            //if(messages.size() > 0 ) {
                //book.update(messages);
                //update_states(current_time);
            //}
            //return nmessages;
        //}
        int process() {
            std::string line;
            std::vector<Message> messages;

            while(std::getline(*stream,line)) {
                nmessages++;
                OrderTokens tokens = tokenize(line);
                Message m(tokens);
                m = Message(tokens);
                messages.push_back(m);
                book.update(messages);
                update_states(m.timestamp);
                messages.clear();
            }
            if(messages.size() > 0 ) {
                book.update(messages);
                update_states(messages[0].timestamp);
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

    std::cout << std::fixed;
    std::cout.precision(2);
    //std::cin.sync_with_stdio(false);
    //std::cout.sync_with_stdio(false);

    TransactionManager t(target_size,std::cin);
    
    clock_t start, end;
    start=clock();
    uint64_t nmessages = t.process();
    end=clock();

    double total_time = (double)(end-start)/CLOCKS_PER_SEC;
    //std::cout << "processed " << nmessages << " in " << total_time <<
        //" seconds" << std::endl;
    //std::cout << "Average process time per record: " <<
        //(total_time/double(nmessages))*1000 << "ms" << std::endl;
    //std::cout << "Speed: " << nmessages / total_time <<
        //" records per second" << std::endl;
    return 0;
}
