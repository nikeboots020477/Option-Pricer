#include "control.h"


template <typename T>
void MonteCarloPricer<T>::compute_rate_sigma(Data * dt){
spot = dt->Data.back().second;
double mu=0;
double sum_ui=0;
double sum_ui2=0;
double n=dt->Data.size();
for (int i=0;i< n-1;i++) {
    double R;
    double delta_Si_0;
    double Si_0;
    double Si_1;
    Si_0 = dt->Data[i].second;
    Si_1 = dt->Data[i+1].second;
    delta_Si_0 = Si_1 - Si_0;
    R = delta_Si_0 / Si_0;
    mu=mu+R;
    double ui = std::log(Si_1/Si_0);
    sum_ui = sum_ui + ui;
    sum_ui2 = sum_ui2 + std::pow(ui,2);
}
double s = std::sqrt(  sum_ui2 / (n-1) - std::pow(sum_ui,2)/ (n*(n-1))    );

rate = (mu/(n-1)) *252;
sigma = s * std::sqrt(252);

std::cout<<"RATE AND VOLATILITY COMPUTED: "<<"RATE "<<rate<<"  "<<"VOLATILITY "<<sigma<<std::endl;
}

template <typename T>
Result MonteCarloPricer<T>::price(Data * data) {
    compute_rate_sigma(data);
    int num_threads = std::thread::hardware_concurrency();
    int n_simulations_thread = n_simulations / num_threads;

    std::vector<std::thread> threads;
    std::vector<Result> results(num_threads);

    for (int i = 0; i < num_threads; ++i) {
        // Use a lambda to capture `this` and pass the member function
        threads.emplace_back([this, i, n_simulations_thread, &results]() {
            this->thread_price(n_simulations_thread, results[i]);
        });
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Aggregate results
    result.price = 0;
    result.std = 0;
    for (auto& r : results) {
        result.price += r.price;
        result.std += r.std * r.std;
    }

    result.price /= num_threads;
    result.std = sqrt(result.std / num_threads);
    std::cout<<"Price calculated"<<"\n";
    return result;
}

template <typename T>
void MonteCarloPricer<T>::thread_price(int num_of_simulations, Result& mc_result) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> d(0., 1.);

    for (int j = 0; j < num_of_simulations; ++j) {
        double log_spot = log(spot);
        double dt = product.get_maturity() / n_time_steps;

        for(int i = 0; i < n_time_steps; ++i) {
            double z = d(gen);
            log_spot = log_spot + (rate - 0.5 * sigma * sigma) * dt + sigma * sqrt(dt) * z;
        }

        double payoff = product.payoff(exp(log_spot));
        mc_result.price += payoff;
        mc_result.std += payoff * payoff;
    }

    mc_result.price /= num_of_simulations;
    mc_result.std /= num_of_simulations;
    mc_result.std -= mc_result.price * mc_result.price;
    mc_result.std = sqrt(mc_result.std);
}

Request::Request(const std::string r){
    std::regex pattern(R"(^(OPTION)\s*(CALL|PUT)\s+(MC)\s+(\d+(\.\d+)?)\s+(\d+(\.\d+)?)$)");
        std::smatch matches;
        if (std::regex_match(r, matches, pattern)) {
            predictor_type = matches[1];
            option_type = matches[2];
            strike = std::stod(matches[4]);
            maturity = std::stod(matches[6]);
            pricer = matches[3];
            std::cout<<"COMMAND PARSED."<<std::endl;
    }else{
        throw MyException();
    }
}

Request::Request(int argc, char *argv[]){
    char buff[50];
    strcat(buff, argv[1]);
    for(int i=2; i<argc; i++){
        strcat(buff, argv[i]);
    }
    std::regex pattern(R"(^(CALL|PUT)\s*(MC)\s*(\d+(\.\d+)?)\s*(\d+(\.\d+)?)$)");
    std::smatch matches;
    std::string strr= std::string(buff);
    if( std::regex_match(strr, matches, pattern) ) {
        option_type = matches[1];
        strike = std::stod(matches[3]);
        maturity = std::stod(matches[5]);
        pricer = matches[2];
        std::cout<< matches[1]<<"  " << matches[2] <<"  " << std::stod(matches[3])<<"  "<< std::stod(matches[5])<<"  ";
    }else{
        throw MyException();
    }
}


void Controller ::handle_request(Request request){
Result res;
 std::cout<<"Request handling\n";
DBUpload * _DB_upload = new PostgreSQLUpload(); 
Data * _Data = _DB_upload->upload("sber"); 
PricerFactory* _PricerFactory; 
Pricer* _Pricer ;
 
if (request.get_option_type() == "CALL")
{
    std::cout<<"MAKING CALL OPTION "<<request.get_option_type()<<"\n";
    _Pricer = _PricerFactory->create_pricer(request, EuropeanCallOption( request.get_strike(), request.get_maturity() ) ); 
}
else
{	std::cout<<"MAKING PUT OPTION "<<request.get_option_type()<<"\n";
    _Pricer = _PricerFactory->create_pricer(request, EuropeanPutOption( request.get_strike(), request.get_maturity() ) );  
}
if(_Pricer) {
    if(_Data){
        res = _Pricer->price(_Data);
        std::cout <<"Option prise is "<< res.price<<", "<<"std is "<<res.std<<std::endl;
    }else{
        std::cout<<"_Data is nullptr\n";
    }
}else{
    std::cout<<"_Pricer is nullptr\n";}

delete _DB_upload;
delete _Data;
delete _Pricer;

}



Pricer * PricerFactory::create_pricer(Request req, EuropeanCallOption prod){
    if (req.get_pricer()== "MC"){    
            Pricer * pr= new MonteCarloPricer<EuropeanCallOption>(prod);  
            return pr;
    }
    std::cout<<"create_pricer returned nullptr: INNCORRECT PRICER"<<std::endl;
    return nullptr;
}


Pricer * PricerFactory::create_pricer(Request req, EuropeanPutOption prod){
    if (req.get_pricer()== "MC"){     
            Pricer * pr= new MonteCarloPricer<EuropeanPutOption>(prod); 
            return pr; 
    }
    std::cout<<"create_pricer returned nullptr: INNCORRECT PRICER"<<std::endl;
    return nullptr;
}

void PostgreSQLUpload::insert_to_char_array(char const * strA, char const* strB,char *strC, int x ){
    //strC[0]='\0';
    strncpy(strC, strA, x);
    strC[x] = '\0';
    strcat(strC, strB);
    strcat(strC, strA + x);
}


Data* PostgreSQLUpload::upload(const char * table_name){
    Data *data = new Data;
    int n_fields;
    int i;
    int j;
   
    PGconn *connection;
    PGresult *result;
    
    connection = PQconnectdb(connection_info);
    if (PQstatus(connection) != CONNECTION_OK)
    {
        fprintf(stderr, "Error message: %s", PQerrorMessage(connection));
        PQfinish(connection);
        return nullptr;
        
    }

    char select[50];
    insert_to_char_array("SELECT date,close FROM ;", table_name, select, 23);

    result = PQexec(connection, select);
    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "SELECT failed. Error message: %s", PQerrorMessage(connection));
        PQfinish(connection);
        return nullptr;
    }

    n_fields = PQnfields(result);
    
    for (i = 0; i < PQntuples(result); i++)
    {
        std::pair<int,double> tmp;

        tmp.first=std::stoi(PQgetvalue(result, i, 0));
        tmp.second=std::stod(PQgetvalue(result, i, 1));
        /*for (j = 0; j < n_fields; j++)
        {
        //tmp.push_back(  std::stod( std::string( PQgetvalue(result, i, j) ) ) );
        }*/
        data->Data.push_back(tmp);
    }
    PQclear(result);
    PQfinish(connection);

    return data;
}
