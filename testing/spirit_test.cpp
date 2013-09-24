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
#include <ctime>
#include <iterator>

namespace client
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
    client::message,
    (int, timestamp)
    (char, mtype)
    (std::string, id)
    (char, otype)
    (double, price)
    (int, quantity)
)

namespace client
{
    template <typename Iterator>
    struct message_parser : qi::grammar<Iterator, message(), ascii::space_type>
    {
        message_parser() : message_parser::base_type(start)
        {
            using qi::int_;
            using qi::double_;
            using ascii::char_;

            start %= qi::hold[int_
                >>  char_
                >>  qi::as_string[qi::lexeme[+~char_(' ')]]
                >>  char_
                >>  double_
                >>  int_]
                |
                int_
                >>  char_
                >>  qi::as_string[qi::lexeme[+~char_(' ')]]
                >>  qi::attr('0')
                >>  qi::attr(-1)
                >>  int_
                ;
        }
        qi::rule<Iterator, message(), ascii::space_type> start;
    };
}

int
main()
{
    using boost::spirit::ascii::space;
    typedef std::string::const_iterator iterator_type;
    typedef client::message_parser<iterator_type> message_parser;

    message_parser g;
    std::string str;
    str.reserve(32);

    unsigned int nlines(0);

    clock_t start, end;
    double total_time;
    start=clock();
    std::ios_base::sync_with_stdio (false);
    std::vector<client::message> messages;
    messages.reserve(2e6);

    while (getline(std::cin, str))
    {
        client::message message;
        std::string::const_iterator iter = str.begin();
        std::string::const_iterator end = str.end();
        bool r = phrase_parse(iter, end, g, space, message);
        messages.push_back(message);
        
        if (r && iter == end) {
            nlines++;
        } else {
            std::cerr << "Failed to parse line: " << str << std::endl;
        }
    }

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
