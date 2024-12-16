#ifndef PRODUCTS_HPP
#define PRODUCTS_HPP

#include <algorithm>


class EuropeanOption {
public:
    virtual double payoff(double spot) const = 0;
    virtual double get_maturity() const =0;
    virtual ~EuropeanOption() = default;
};

// Curiously recursive template pattern
class EuropeanCallOption : public EuropeanOption {
private:
    double price ;
    double strike;
    double maturity;

public:
    EuropeanCallOption(){};
    EuropeanCallOption(double strike, double maturity) : strike(strike), maturity(maturity) {}

    double payoff(double spot) const override {
        return std::max(spot - strike, 0.);
    };

    double get_maturity() const override{ return maturity; };
};

class EuropeanPutOption : public EuropeanOption {
private:

    double strike;
    double maturity;

public:
    EuropeanPutOption(){};
    EuropeanPutOption(double strike, double maturity) : strike(strike), maturity(maturity) {}

    double payoff(double spot) const override {
        return std::max(strike - spot, 0.);
    };

    double get_maturity() const override{ return maturity; };
};


#endif // PRODUCTS_HPP
