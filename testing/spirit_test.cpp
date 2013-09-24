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
//#include <boost/phoenix/object/static_cast.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <map>

#include <ctime>

namespace pricer
{
    namespace qi = boost::spirit::qi;
    namespace ascii = boost::spirit::ascii;

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

            start %= qi::hold[int_
                >>  char_
                >>  as_string[lexeme[+~char_(' ')]]
                >>  char_
                >>  double_
                >>  int_]
                |
                int_
                >>  char_
                >>  as_string[lexeme[+~char_(' ')]]
                >>  attr('0')
                >>  attr(-1)
                >>  int_
                ;
        }
        qi::rule<Iterator, message(), ascii::space_type> start;
    };

    struct order {
        char otype;
        double price;
        int quantity;

        order(const char &t, const double &p, const int &q) : otype(t), price(p), quantity(q) {}
    };

    struct orderbook {
        typedef std::unordered_map<std::string, std::shared_ptr<order> > Book;
        typedef std::multimap<int, std::shared_ptr<order> > BookIndex;
        Book orders;
        BookIndex asks, bids;
        double minimum_selling_price, maximum_buying_price;

        orderbook() : minimum_selling_price(-1), maximum_buying_price(0) {
        }

        bool process_message(const message &m) {
            if(m.mtype=='A') {
                // processing addition 
                auto o = std::make_shared<order>(m.otype,m.price,m.quantity);
                orders.insert({m.id,o});
                if(m.otype=='A') {
                    asks.insert({static_cast<int>(m.price*100),o});
                } else {
                    bids.insert({static_cast<int>(m.price*100),o});
                }
            } else {
                Book::iterator o = orders.find(m.id);
                //int prev_quantity = o->second->quantity;
                //o->second->quantity = prev_quantity < m.quantity ? 0 : prev_quantity - m.quantity;
                //if( prev_quantity < m.quantity) { // need to remove it from teh indices
                    
                //}
                
            }
            return true;
        }

        int process_messages(const std::vector<message> &messages) {
            int nmess = 0;
            for(const auto &m: messages) {
                nmess += process_message(m);
            }
            return nmess;
        }
    };
}

unsigned int
fetch_messages(std::vector<pricer::message>* messages, int nmessages=2e6) {
    using boost::spirit::ascii::space;
    typedef std::string::const_iterator iterator_type;
    typedef pricer::message_parser<iterator_type> message_parser;

    message_parser g;

    messages->reserve(nmessages);
    std::string str;
    str.reserve(32);

    unsigned int parsed_messages(0);
    std::ios_base::sync_with_stdio (false);
    while (getline(std::cin, str))
    {
        pricer::message message;
        std::string::const_iterator iter = str.begin();
        std::string::const_iterator end = str.end();
        bool r = phrase_parse(iter, end, g, space, message);
        messages->push_back(message);
        
        if (r && iter == end) {
            parsed_messages++;
        } else {
            std::cerr << "Failed to parse line: " << str << std::endl;
        }
    }
    return parsed_messages;
}

int
main()
{
    std::vector<pricer::message> messages;

    clock_t start, end;
    double total_time;

    pricer::orderbook book;
    start=clock();
    fetch_messages(&messages);
    unsigned int nlines = book.process_messages(messages);
    end=clock();

    total_time = (double)(end-start)/CLOCKS_PER_SEC;

    std::cout << "Parsed " << nlines << " lines successfully" << std::endl;
    std::cout << "Took " << total_time << " seconds" << std::endl;
    std::cout << (double)(nlines / total_time) << " lines per second" << std::endl;

    std::cout << "Final Line"  <<  std::endl <<
                 "----------"  <<  std::endl <<
                 "Timestamp: " <<  messages.back().timestamp << std::endl <<
                 "mtype:     " <<  messages.back().mtype     << std::endl <<
                 "id:        " <<  messages.back().id        << std::endl <<
                 "otype:     " <<  messages.back().otype     << std::endl <<
                 "quantity:  " <<  messages.back().quantity  << std::endl;

    return 0;
}
