/* g++ -std=c++11 -march=native -O2 -o spirit.x spirit_test.cpp */

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_hold.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <map>

#include <cstdlib>
#include <cmath>

#include <ctime>

namespace pricer
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

    enum class action { none=0, buy, sell };

    struct message
    {
        int timestamp;
        char mtype;
        std::string id;
        char otype;
        double price;
        int quantity;
    };
}

BOOST_FUSION_ADAPT_STRUCT(
    pricer::message,
    (int, timestamp)
    (char, mtype)
    (std::string, id)
    (char, otype)
    (double, price)
    (int, quantity)
)

namespace pricer
{
    template <typename Iterator>
    struct message_parser : qi::grammar<Iterator, message(), ascii::space_type>
    {
        message_parser() : message_parser::base_type(start)
        {
            using qi::int_;
            using qi::double_;
            using qi::as_string;
            using qi::lexeme;
            using qi::attr;
            using ascii::char_;
            using qi::_val;

            // Define grammary for two possible message syntaxes
            start %= qi::hold[
            // timestamp  type       stock ID                       order type  price     quantity
                int_ >> char_ >> as_string[lexeme[+~char_(' ')]] >> char_     >> double_ >> int_ ]
                |
                int_ >> char_ >> as_string[lexeme[+~char_(' ')]] >> attr('0') >> attr(-1) >> int_
                ;
        }
        qi::rule<Iterator, message(), ascii::space_type> start;
    };

    struct order {
        char type;
        double price;
        int quantity;
        std::string id;

        order(const char &t, const double &p, const int &q, const std::string &s) : type(t),
                price(p), quantity(q), id(s) {}
    };

    struct orderbook {
        typedef std::unordered_map<std::string, std::shared_ptr<order> > Book;
        typedef std::multimap<int, std::shared_ptr<order> > BookIndex;
        
        int target_size, total_asks, total_bids;
        double minimum_selling_price, maximum_buying_price, total_sales, total_purchases;

        Book orders;
        BookIndex asks, bids;
        
        std::ostringstream outstream;

        orderbook(int ts) : minimum_selling_price(0), maximum_buying_price(0), total_asks(0),
                total_bids(0), total_sales(0), total_purchases(0), target_size(ts) {
            outstream.precision(2);
            outstream << std::fixed;
        } 

        // print the contents of outstream to stdout and reset it
        void communicate();
        // check asks and add notify via outstream if a new optimal solution exists
        void buy(int);
        // check bids and add notify via outstream if a new optimal solution exists
        void sell(int);
        // update orders and asks/bids with new stock - returns if buy/sell
        // needs to be processed
        action process_add_order(const message &);
        // update orders and asks/bids with removal of stock - returns if buy/sell
        // needs to be processed
        action process_removal_order(const message &);
        // determine type of message (A / R) and process; call buy/sell if
        // required
        void process_message(const message &);
        // collection processor using process_message; calls communicate() when
        // each time the integer argument number of messages have been processed
        int process_messages(const std::vector<message> &, int);
    };

    void orderbook::communicate() {
        if( outstream.tellp() > 0 ) {
            std::cout << outstream.str() << std::flush;
            outstream.str(std::string());
        }
    }

    void orderbook::buy(int timestamp) {
        bool sufficient_stock = total_asks >= target_size;
        bool have_bought = total_purchases > 0;
        if(!sufficient_stock && have_bought) {
            maximum_buying_price = 0;
            total_purchases = 0;
            outstream << timestamp << " B NA" <<  std::endl;
        } else if(sufficient_stock) {
            BookIndex::iterator it = asks.begin(); 
            int bought(0);
            double cost(0);
            double max_price(0);
            while(it != asks.end() && bought < target_size) {
                int quantity = it->second->quantity;
                int to_buy = std::min(target_size - bought, quantity);
                cost += to_buy * (it->second->price);
                max_price = std::max(it->second->price, max_price);
                bought += to_buy;
                ++it;
            }
            if(floor(cost*100 + 0.5) - floor(total_purchases*100 + 0.5) != 0) {
                total_purchases = cost;
                maximum_buying_price = max_price;
                outstream << timestamp << " B " << total_purchases << std::endl;
            }
        }
    }

    void orderbook::sell(int timestamp) {
        bool sufficient_stock = total_bids >= target_size;
        bool have_sold = total_sales > 0;

        if(!sufficient_stock && have_sold) {
            total_sales = 0;
            minimum_selling_price = 0;
            outstream << timestamp << " S NA" <<  std::endl;
        } else if(sufficient_stock) { 
            BookIndex::reverse_iterator it = bids.rbegin(); 
            int sold(0);
            double takings(0);
            double min_price(it->second->price);
            while(it != bids.rend() && sold < target_size) {
                int quantity = it->second->quantity;
                int to_sell = std::min(target_size - sold, quantity);
                takings += to_sell * (it->second->price);
                min_price = std::min(it->second->price, min_price);
                sold += to_sell;
                ++it;
            }
            int diff = floor(takings*100. + 0.5) - floor(total_sales*100. + 0.5);
            if(diff !=0) {
                total_sales = takings;
                minimum_selling_price = min_price;
                outstream << timestamp << " S " << total_sales << std::endl;
            }
        }
    }

    action orderbook::process_add_order(const message &m) {
        bool do_buy(false), do_sell(false);
        // processing addition 
        auto o = std::make_shared<order>(m.otype,m.price,m.quantity,m.id);
        orders.insert({m.id,o});
        int idx = floor(m.price*100 + 0.5);
        if(m.otype=='S') {
            asks.insert({idx,o});
            total_asks += o->quantity;
            do_buy |= m.price < maximum_buying_price || maximum_buying_price == 0;
        } else {
            bids.insert({idx,o});
            total_bids += o->quantity;
            do_sell |= m.price > minimum_selling_price || minimum_selling_price == 0;
        }
        if(do_buy) return action::buy;
        if(do_sell) return action::sell;
        return action::none;
    }

    action orderbook::process_removal_order(const message &m) {
        bool do_buy(false), do_sell(false);
        // removal
        bool clean_order(false);
        Book::iterator id_order = orders.find(m.id);
        double price = id_order->second->price;
        int order_quantity(id_order->second->quantity);
        int deduction(m.quantity);
        if(order_quantity <= m.quantity) { // fully remove from indices
            clean_order = true;
            deduction = order_quantity;
            int k = floor(price*100 + 0.5);
            BookIndex *index = (id_order->second->type == 'S') ? &asks : &bids;
            auto match = index->equal_range(k);
            for(auto it=match.first; it!= match.second; ++it) { // remove matching
                if(it->second->id == m.id) {
                    index->erase(it);
                    break;
                }
            }
        } else {
            id_order->second->quantity -= deduction;
        }
        if(id_order->second->type == 'S') {
            total_asks -= deduction;
            do_buy |=  price <= maximum_buying_price && total_purchases > 0;
        } else {
            total_bids -= deduction;
            do_sell |= price >= minimum_selling_price && total_sales > 0;
        }
        if(clean_order) orders.erase(id_order);
        
        if(do_buy) return action::buy;
        if(do_sell) return action::sell;
        return action::none;
    }

    void orderbook::process_message(const message &m) {
        action a;
        if(m.mtype=='A') {
            a = process_add_order(m);
        } else {
            a = process_removal_order(m);
        }
        if(a==action::buy) {
            buy(m.timestamp);
        } else if(a==action::sell) {
            sell(m.timestamp);
        }
    }

    int orderbook::process_messages(const std::vector<message> &messages, int buffer_messages=1000){
        int nmess = 0;
        for(const auto &m: messages) {
            process_message(m);
            nmess++;
            if( nmess % buffer_messages == 0 ) {
                communicate();
            }
        }
        communicate();
        return nmess;
    }
}

std::ostream& operator<< (std::ostream& os, const pricer::message& m) {
    return os << "(" << m.timestamp << " " << m.mtype << " " << m.id << " " << 
        m.otype << " " << m.price << " " << m.quantity << ")";
}

unsigned int
fetch_messages(std::vector<pricer::message>* messages, unsigned int nmessages=2e6) {
    using boost::spirit::ascii::space;
    typedef std::string::const_iterator iterator_type;
    typedef pricer::message_parser<iterator_type> message_parser;

    message_parser g;

    messages->reserve(nmessages);
    std::string str;
    str.reserve(32);

    unsigned int fetched(0);
    unsigned int parsed_messages(0);
    while (fetched < nmessages && getline(std::cin, str))
    {
        pricer::message message;
        std::string::const_iterator iter = str.begin();
        std::string::const_iterator end = str.end();
        bool r = phrase_parse(iter, end, g, space, message);
        if (r && iter == end) {
            fetched++;
            messages->push_back(message);
            parsed_messages++;
        } else {
            std::cerr << "Failed to parse line: " << str << std::endl;
        }
    }
    return parsed_messages;
}

int
main(int argc, char** argv)
{
    if(argc < 2) {
        std::cout << "This program takes 1 argument" << std::endl;
        return -1;
    }
    int target_size = atoi(argv[1]);

    std::ios_base::sync_with_stdio (false);
    std::cout << std::fixed;
    std::cout.precision(2);

    std::vector<pricer::message> messages;

    clock_t start, end;
    double total_time;

    unsigned int nmessages(0);

    start=clock();
    pricer::orderbook book(target_size);
    while(fetch_messages(&messages,1e3) > 0)  {
        nmessages += book.process_messages(messages);
        messages.clear();
    }
    end=clock();

    total_time = (double)(end-start)/CLOCKS_PER_SEC;
    std::cout << "Took " << total_time << " seconds" << std::endl;

    return 0;
}
