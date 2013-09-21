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
typedef std::map<std::string, Order*> BookLookup;
typedef std::pair<std::string, Order*> BookLookupEntry;
typedef std::map<std::string, Order> Book;
typedef std::pair<std::string, int> Transaction;

enum class MessageType : char { ADD='A', REDUCE='R' };
enum class OrderType : char { BID='B', ASK='S', NA };

// take a string and split by spaces 
OrderTokens tokenize(std::string& line) {
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
        // the position of the order size changes dependign on the type
        int size_pos = (mtype == MessageType::ADD) ? 5 : 3;
        size = atoi(tokens[size_pos].c_str());
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
        std::cout << "[" << char(type) <<  " " << size << " " << price <<
            "]" << std::endl;
    }
};

struct LookupCompare {
        // predicate function: sort iterable of pairs of <ID, Order> by
        // increasing price.
        bool operator()(const BookLookupEntry &lhs, const BookLookupEntry &rhs){
            return lhs.second->price < rhs.second->price;
        }
};


struct OrderBook {
    private:
        // require this to be private because "asks" and "bids" reference
        // memory owned by orders.  Therefore we require that OrderBook handles
        // the management of this memory such that the pointers in "asks" and
        // "bids" do not become invalid
        Book orders;
    public:
        BookLookup asks, bids;
        // this allows us to choose whether we look at "asks" or "bids"
        // depedning on the order type we are considering
        std::map<OrderType, BookLookup*> lookup_reference;
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
                // hilarious unreadable one liner: (*l)[m.ID] = &(orders[m.ID]);
                BookLookup * l = lookup_reference[m.otype];
                // l is a pointer to either a book of BUYs or SELLs (see default
                // ctor for OrderBook). 
                // We then make the *value* of the map buy/sell [ID] be a
                // pointer to the new record in OrderBook.orders
                Order* o_ptr = &(orders[m.ID]);
                l->insert({m.ID,o_ptr});
                if(o.type == OrderType::ASK) total_asks += o.size;
                else if(o.type == OrderType::BID) total_bids += o.size;
            } else {
                std::cerr << "[ERR] received ADD order for existing ID=" <<
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
                    OrderType t = id_order->second.type;
                    lookup_reference[t]->erase(id_order->first);
                    orders.erase(id_order->first);
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

    double earnings, expenditure;
    bool have_bought, have_sold;

    public:
        TransactionManager(int target, std::istream &input_stream) :
            nmessages(0), target_size(target), stream(&input_stream),
            earnings(0), expenditure(-1), have_bought(false), have_sold(false)
        {}

        double make_sale(int time) {
            std::vector<BookLookupEntry> bid_portfolio(book.bids.begin(),
                    book.bids.end());
            // copy out map into a vector of key:value pairs
            std::sort(bid_portfolio.begin(),bid_portfolio.end(),
                    LookupCompare());
            int total_sold = 0;
            double total_takings = 0;
            std::vector<BookLookupEntry>::iterator p = bid_portfolio.end();
            while(total_sold < target_size) {
                p--;
                int available = p->second->size;
                double price = p->second->price;
                // only sell as much as we need / is available
                int selling = std::min(available, target_size-total_sold);
                total_sold += selling;
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
            static double diff(0);
            std::vector<BookLookupEntry> ask_portfolio(book.asks.begin(),
                    book.asks.end());
            std::sort(ask_portfolio.begin(),ask_portfolio.end(),
                    LookupCompare());
            int total_bought = 0;
            double total_price = 0;
            std::vector<BookLookupEntry>::iterator p = ask_portfolio.begin();
            while(total_bought < target_size) {
                int available = p->second->size;
                double price = p->second->price;
                // only buy as much as we need / is available
                int buying = std::min(available, target_size-total_bought);
                total_bought += buying;
                total_price += buying * price;
                p++;
            }
            // we care about the *accumulated* change since we last printed
            // a price point. once we have flucated enough to make a rounding
            // change (0.005) we know that we need to print a new value
            diff = fabs(total_price - expenditure);
            have_bought = true;
            if(diff >= 0.005) {
                expenditure = total_price;
                std::cout << time << " B " << expenditure << std::endl;
                diff=0;
            }
            return total_price;
        }

        void update_positions(int time) {
            // we want to reset if we've bought/sold before and the market no
            // longer supports our target_size
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
            // if there is sufficienty supply/demand in the mark buy/sell
            if(book.total_asks >= target_size) {
                make_purchase(time);
            }
            if(book.total_bids >= target_size) {
                make_sale(time);
            }
        }

        int process() {
            std::string line;
            while(std::getline(*stream,line)) {
                nmessages++;
                OrderTokens tokens = tokenize(line);
                // check our line contains sufficient data to form a message
                if(tokens.size() >= 4 ) { 
                    Message m(tokens);
                    // does our message correspond to actual stock?
                    // strol is more robust for non-string cases but is slower
                    // given the error checking here atoi/l is sufficient
                    if(m.size > 0) {
                        book.update(m);
                        update_positions(m.timestamp);
                    } else {
                        std::cerr << "[ERR] malformed message: size 0" <<
                            std::endl;
                    }
                } else {
                    std::cerr << "[ERR] malformed message in line: skipping" <<
                        std::endl;
                }
            }
            return nmessages;
        }

};

int main(int argc, char** argv) {
    // Check that we have an argument we can try to handle as an input
    if(argc < 2) {
        std::cout << "This program takes 1 argument" << std::endl;
        return -1;
    }
    int target_size = atoi(argv[1]);

    std::cout << std::fixed;
    std::cout.precision(2);

    TransactionManager t(target_size,std::cin);
    
    clock_t start, end;
    start=clock();
    uint64_t nmessages = t.process();
    end=clock();

    double total_time = (double)(end-start)/CLOCKS_PER_SEC;
    std::cout << "processed " << nmessages << " in " << total_time <<
        " seconds" << std::endl;
    std::cout << (total_time/double(nmessages))*1000 << "ms/record" << std::endl;
    std::cout << (nmessages / total_time)/1000. << " records/ms" << std::endl;
    return 0;
}
