#ifndef CONTROL_H
#define CONTROL_H

#include "libpq-fe.h"
#include <vector>
#include <random>
#include <thread>
#include <cmath>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include <algorithm>

#include "products.hpp"

struct Result
{
    double price = 0.0;
    double std = 0.0;
};

//Data is struct that contains 2 columns of certain data table: first is Date, second is close Price 
struct Data{
    std::vector< std::pair<int,double> > Data;
};

//Exception class which needs to be thrown in case of incorrect command.
class MyException : public std::runtime_error {
public:
  MyException() : std::runtime_error("INCORRECT REQUEST") { }
};



//Class that parses command.
class Request{
    std::string predictor_type;
    std::string option_type;
    std::string pricer;
    double strike;
    double maturity;


public:    
    double get_strike(){return strike;}
    double get_maturity(){return maturity;}
    std::string get_predictor_type(){return predictor_type;}
    std::string get_option_type(){return option_type;}
    std::string get_pricer(){return pricer;}

    Request(int argc, char *argv[]);
    Request(const std::string r);

};

class DBUpload{
public:    
    virtual Data* upload(const char * table_name)=0;
};

class PostgreSQLUpload: public DBUpload{
    const char * connection_info = {"dbname = mydb user = postgres password = postgres"};
    void insert_to_char_array(char const * strA, char const* strB,char *strC, int x );
public:    
    Data* upload(const char * table_name);
    friend class Controller;
};


class Controller{
public:
    void handle_request(Request request);
};


class Pricer{
public:    
    virtual Result price(Data *data)=0;
};

template <typename T>
class MonteCarloPricer : public Pricer{
private:
    int n_simulations = 1000;
    int n_time_steps = 100;
    int seed;
    Result result; 
    double spot;
    double rate;
    double sigma;
    T product;
    // Make sure to pass this as a pointer to member function
    void thread_price(int num_of_simulations, Result& mc_result);

public:
    Result price(Data *data);
    MonteCarloPricer(){};
    MonteCarloPricer(T product) : product(product) { };
    void compute_rate_sigma(Data *dt);
    Result get_result() { return result; };

};

class PricerFactory{
public:
    Pricer * create_pricer(Request req, EuropeanCallOption prod);
    Pricer * create_pricer(Request req, EuropeanPutOption prod);
};



#endif // CONTROL_H