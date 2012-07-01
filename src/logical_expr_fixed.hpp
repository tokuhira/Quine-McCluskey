
#ifndef LOGICAL_EXPRESSION_HPP
#define LOGICAL_EXPRESSION_HPP


#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <boost/regex.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/dynamic_bitset.hpp>

//
// namespace for Logical Expression
//
namespace logical_expr {


using namespace std;

// Don't care
static const boost::optional<bool> dont_care = boost::none;

// expr_mode is not used now. 
// But this will be used to change the expression of functions
enum expr_mode { alphabet_expr };


class arg_gen_iterator {
public:
    typedef boost::dynamic_bitset<> value_type;
    typedef arg_gen_iterator this_type;
    arg_gen_iterator(int width, int val) : width_(width), current_val_(val) {}
    this_type& operator++() { ++current_val_; return *this; }
    this_type  operator++(int) { this_type it = *this; ++current_val_; return it; }
    this_type& operator--() { --current_val_; return *this; }
    this_type  operator--(int) { this_type it = *this; --current_val_; return it; }
    bool operator<(const this_type &it) const
        { return (it.width_ == width_ && current_val_ < it.current_val_); }
    bool operator>(const this_type &it) const
        { return (it.width_ == width_ && current_val_ > it.current_val_); }
    bool operator==(const this_type &it) const 
        { return (it.width_ == width_ && current_val_ == it.current_val_); }
    bool operator!=(const this_type &it) const 
        { return !(*this == it); }
    value_type operator*() const { return value_type(width_, current_val_); }
private:
    const int width_;
    int current_val_;
};

// Argument generator for logical_function using Iterator (default: arg_gen_iterator)
template<typename Iterator = arg_gen_iterator>
class arg_generator {
public:
    arg_generator(int nbegin, int nend, int width) 
        : begin_(nbegin), end_(nend), width_(width) {}
    Iterator begin() const { return Iterator(width_, begin_); }
    Iterator  end()  const { return Iterator(width_, end_); }
private:
    const int begin_, end_, width_;
};


/*
 * String to be parsed has to be in the following form:
 * ${Function-Name}(Variables-divided-by-',' ...) = ${TERMS} + ...
 * White spaces will be ignored
 * Default character to invert a variable is '^' (first template parameter)
 */
template<char inverter = '^', bool escape = true, expr_mode mode = alphabet_expr>
class function_parser {
public:
    typedef std::pair<string, vector<string>> result_type;
    function_parser() {}
    explicit function_parser(const string &expr) : expr_(expr) {}
    ~function_parser() {}

    void set_expression(const string &expr) { expr_ = expr; }
    const string& get_expression() const { return expr_; }
    const string& function_name() const { return func_name_; }

    result_type parse() {
        auto untokenized = scanner();
        auto token = tokenizer(untokenized);
        use_undeclared_vars(token.second, token.first);
        return std::move(token);
    }

    vector<string> scanner() {
        if( expr_.empty() )
            throw std::runtime_error("expr: Expression is empty, aborted");
        string expr_with_nospaces = boost::regex_replace(expr_, boost::regex("\\s"), "");
        boost::regex reg(
            (boost::format("([A-Za-z_-]+)\\((((\\s*[A-Z],)*)([A-Z]))\\)=(((%1%?[A-Z])+\\+)*((%1%?[A-Z])+))$") 
                % (escape ? string{'\\', inverter} : string{inverter})).str(),
             boost::regex::perl
        );
        boost::smatch result;
        if( !boost::regex_match(expr_with_nospaces, result, reg) )
            throw std::runtime_error("expr: Input string does not match the correct form");
        func_name_ = result[1];
//        for( unsigned int i = 0; i < result.size(); ++i )
//            cout << "result[" << i << "] = " << result[i] << endl;
//                            decl-vars  decl-terms
        return vector<string>{result[2], result[6]};
    }

    static result_type tokenizer(const vector<string> &untokenized) {
        std::vector<string> vars, terms;
        typedef boost::char_separator<char> char_separator;
        boost::tokenizer<char_separator> 
            var_tokenizer(untokenized[0], char_separator(",")), term_tokenizer(untokenized[1], char_separator("+"));
        ostringstream oss;
        for( auto token : var_tokenizer )   oss << token;
        for( auto token : term_tokenizer )  terms.push_back(token);
        return std::make_pair(oss.str(), terms);
    }

    static bool use_undeclared_vars(const vector<string> &terms, const string &vars) {
        for( auto term : terms ) {
          for( auto used_var : term ) {
            if( used_var != inverter && vars.find(used_var, 0) == string::npos )
              throw std::runtime_error(
                (boost::format("expr: Using undeclared variable, %c") % used_var).str()
                );
          }
        }
        return true;
    }

private:
    string expr_, func_name_;
};


//
// Term properties
//
// Type Requirements:
//  [*] Have set() and get() member functions
//  [*] Default constructible
//
class term_no_property {
public:
    typedef int value_type;
    value_type get() const { return 0; }
    void set(value_type) const {}
};

class term_mark {
public:
    typedef bool value_type;
    term_mark() : mark_(false) {}
    value_type get() const { return mark_; }
    void set(bool mark) { mark_ = mark; }
private:
    value_type mark_;
};

class term_number {
public:
    typedef unsigned int value_type;
    term_number() : number_(0) {}
    value_type get() const { return number_; }
    void set(value_type number) { number_ = number; }
private:
    value_type number_;
};
//

template<typename Property = term_no_property, char inverter = '^'>
class logical_term {
public:
    typedef boost::optional<bool> value_type;    
    typedef boost::dynamic_bitset<> arg_type;
    typedef logical_term this_type;
    typedef Property property_type;
    
    logical_term() {}
    logical_term(const string &expr, int bitsize) 
        : expr_(expr), term_(bitsize, logical_expr::dont_care) { parse(expr_); }
    explicit logical_term(const arg_type &arg) {
        for( int i = arg.size() - 1; 0 <= i; --i )
            term_.push_back(arg[i]);
    }

    unsigned int size() const { return term_.size(); }
    const string& get_expr() const { return expr_; }
    const vector<value_type>& get_term() const { return term_; }
    property_type& property() { return property_; }
    const property_type& property() const { return property_; }

    bool size_check(const arg_type &arg) const { return (size() == arg.size()); }
    bool size_check(const this_type &term) const { return (size() == term.size()); }

    int num_of_value(bool value) const {
        int value_count = 0;
        for( auto b : term_ )
            if( b != dont_care && *b == value )
                ++value_count;
        return value_count;
    }

    int diff_size(const this_type &term) const {
        if( !size_check(term) ) return -1;
        int diff_count = 0;
        for( int i = 0; i < size(); ++i ) {
            if( term_[i] != term[i] )
                ++diff_count;
        }
        return diff_count;
    }

    bool calculate(const arg_type &arg) const {
        if( !size_check(arg) )
            throw std::runtime_error("target two operands are not same size");
        bool ret = true;
        for( int i = 0; i < term_.size(); ++i )
            ret = ret && (term_[i] == dont_care ? true : arg[term_.size() - 1 -  i] == term_[i]);
        return ret;
    }

    bool operator()(const arg_type &arg) const { 
        return calculate(arg);
    }

    value_type& operator[](int index) { return term_[index]; }
    const value_type& operator[](int index) const { return term_[index]; }
    
    bool operator==(const logical_term &term) const {
        if( !size_check(term) ) return false;
        arg_generator<> gen(0, std::pow(2, size()), size());
        for( auto it = gen.begin(); it != gen.end(); ++it )
            if( calculate(*it) != term.calculate(*it) )
                return false;
        return true;
    }

private:
    void parse(const string &expr) {
        bool invert = false;
        for( char var : expr ) {
            if( var == inverter ) {
                invert = true;
                continue;
            }
            term_[var - 'A'] = !invert;
            invert = false;
        }
    }
    
    string expr_;
    vector<value_type> term_;
    property_type property_;
};

template<typename Property, char inverter>
typename logical_term<Property, inverter>::property_type::value_type
    property_get(const logical_term<Property, inverter>& term) { return term.property().get(); }

template<typename Property, char inverter>
void property_set(logical_term<Property, inverter>& term,
    typename logical_term<Property, inverter>::property_type::value_type arg)
{ return term.property().set(arg); }

template<typename Prop, char Inv>
logical_term<Prop, Inv> onebit_minimize(const logical_term<Prop, Inv> &a, const logical_term<Prop, Inv> &b)
{
    if( a.size() != b.size() || 1 < a.diff_size(b) )
        throw std::runtime_error("tried to minimize two different size terms");
    logical_term<Prop, Inv> term(a);
    for( int i = 0; i < term.size(); ++i )
        if( term[i] != b[i] )
            term[i] = dont_care;
    return std::move(term);
}


}   // namespace logical_expr

template<typename Prop, char Inv>
std::ostream& operator<<(std::ostream &os, const logical_expr::logical_term<Prop, Inv> &bf) {
        for( auto b : bf.get_term() ) {
            if( b ) os << *b;
            else    os << 'x';
        }
        return os;
}


namespace logical_expr {


template<typename TermType>
class logical_function {
public:
    typedef TermType value_type;
    typedef boost::dynamic_bitset<> arg_type;
    typedef typename vector<value_type>::iterator iterator;
    typedef typename vector<value_type>::const_iterator const_iterator;
    typedef typename logical_term<TermType> this_type;

    logical_function() {}
    explicit logical_function(const TermType &term) { add(term); }
    ~logical_function() {}

    iterator begin() { return func_.begin(); }
    const_iterator begin() const { return func_.begin(); }
    iterator end() { return func_.end(); }
    const_iterator end() const { return func_.end(); }

    int size() const { return func_.size(); }
    int term_size() const {
        if( size() == 0 ) return 0;
        return func_[0].size();
    }
    void add(const TermType &term) { func_.push_back(term); }
    void add(const this_type &func) {
        for( auto term : func )
            add(term);
    }

    void clear() { func_.clear(); }
    
    bool calculate(const arg_type &arg) const {
        bool ret = false;
        for( auto term : func_ )
            ret = ret || term(arg);
        return ret;
    }

    bool operator()(const arg_type &arg) const { return calculate(arg); }
    value_type& operator[](int index) { return func_[index]; }
    const value_type&  operator[](int index) const { return func_[index]; }
    logical_function operator+(const TermType &term) {
        logical_function ret(*this);
        ret += term;
        return ret;
    }
    logical_function  operator+(const logical_function &func) {
        logical_function ret(*this);
        ret += func;
        return ret;
    }
    logical_function& operator+=(const TermType &term) { add(term); return *this; }
    logical_function& operator+=(const logical_function &func) { add(func); return *this; }
    friend ostream& operator<<(ostream &os, const logical_function &bf) {
        for( auto term : bf.func_ )
            os << term << " ";
        return os;
    }
    bool operator==(const logical_function<logical_term<>> &func) const {
        arg_generator<> gen(0, std::pow(2, func.term_size()), func.term_size());
        for( auto it = gen.begin(); it != gen.end(); ++it )
            if( calculate(*it) != func.calculate(*it) )
                return false;
        return true;
    }

private:
    vector<TermType> func_;
};


}   // namespace logical_expr


template<typename Prop, char Inv>
logical_expr::logical_function<logical_expr::logical_term<Prop, Inv>> operator+
    (const logical_expr::logical_term<Prop, Inv> &first, const logical_expr::logical_term<Prop, Inv> &second) {
    logical_expr::logical_function<logical_expr::logical_term<Prop, Inv>> ret(first);
    ret += second;
    return ret;
}


 
#endif  // LOGICAL_EXPRESSION_HPP
