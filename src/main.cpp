
#include <iostream>
#include <string>
#include <utility>
#include <stdexcept>
#include <cmath>
#include <cstdlib>
#include "logical_expr.hpp"
#include "quine_mccluskey.hpp"

using namespace std;

template<typename TermType>
void print_truth_table(const logical_expr::logical_function<TermType> &f) {
    cout << endl << "Truth Table: ";
    for( auto term : f )
        cout << term << " + ";
    cout << "\b\b\b" << endl;
    for( char c = 'A'; c != 'A' + f.term_size(); ++c )
        cout << c;
    cout << " | f()"<< endl;
    for( int i = 0; i < f.term_size() + 6; ++i )
        cout << ((i == f.term_size() + 1) ? '|' : '-');
    cout << endl;
    logical_expr::arg_generator<> gen(0, std::pow(2, f.term_size()), f.term_size());
    for( auto it = gen.begin(); it != gen.end(); ++it )
        cout << *it << " |  " << f(*it) << endl;
}

void print_bit_table(const quine_mccluskey::simplifier::table_type &table)
{
    int true_count = 0;
    for( auto set : table ) {
        cout << "true_count = " << true_count++ << endl;
        for( auto term : set )
            cout << term << " ";
        cout << endl;
    }
}

int main(int argc, char **argv)
{
    int exit_code = EXIT_SUCCESS;
    try {
        cout << "Quine-McCluskey" << endl
             << "[*] Enter a logical expression" << endl
             << "Input: " << flush;
        string line;
        getline(cin, line);
        // Parse input logical expression and return tokenized
        auto token = logical_expr::function_parser<'^', true>(line).parse();

        // Create a logical function using logical_term<term_make>
        typedef logical_expr::logical_term<logical_expr::term_mark> TermType;
        logical_expr::logical_function<TermType> function;
        for( string term : token.second )
            function += logical_expr::parse_logical_term<logical_expr::term_mark>(term, token.first.size());

        // Create a simplifier using Quine-McCluskey algorithm
        quine_mccluskey::simplifier qm(function);
        print_truth_table(qm.get_std_spf());  // Print sum of products form
        cout << endl;
        qm.compress_table();
        cout << endl << "Prime implicants: " << endl;
        qm.print_prime_implicants();
        //print_truth_table(qm.simplify());
    }
    catch( std::exception &e ) {
        cerr << endl << "[-] Exception: " << e.what() << endl;
        exit_code = EXIT_FAILURE;
    }
    return exit_code;
}

