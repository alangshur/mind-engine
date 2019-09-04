#include <iostream>
#include <algorithm>
#include "util/iqr-scorer.hpp"
using namespace std;

void IQRScorer::add_even_quartile_it(quartile_t& quartile, 
    multiset<uint32_t>& quartile_set, bool less_than_quartile) {

    // handle quartile-sample placement
    if (less_than_quartile) {
        quartile = {
            double(*prev(quartile.it_f) + *(quartile.it_f)) / 2.0,
            true,
            prev(quartile.it_f),
            quartile.it_f
        };
    }
    else {
        quartile = {
            double(*(quartile.it_f) + *next(quartile.it_f)) / 2.0,
            true,
            quartile.it_f,
            next(quartile.it_f)
        };
    }
}

void IQRScorer::add_odd_quartile_it(quartile_t& quartile, 
    multiset<uint32_t>& quartile_set, bool less_than_quartile) {

    // handle quartile-sample placement
    if (less_than_quartile) {
        quartile = {
            double(*(quartile.it_f)),
            false,
            quartile.it_f,
            quartile_set.end()
        };
    }
    else {
        quartile = {
            double(*next(quartile.it_f)),
            false,
            next(quartile.it_f),
            quartile_set.end()
        };
    }
}

void IQRScorer::add_quartile(uint32_t sample, quartile_t& quartile,
    multiset<uint32_t>& quartile_set) {
    quartile_set.insert(sample);

    // update specified set quartile 
    if (quartile_set.size() % 2) {
        if (double(sample) >= *(quartile.it_f)) 
            this->add_odd_quartile_it(quartile, quartile_set, false);
        else this->add_odd_quartile_it(quartile, quartile_set, true);
    }
    else {
        if (double(sample) >= *(quartile.it_f)) 
            this->add_even_quartile_it(quartile, quartile_set, false);
        else this->add_even_quartile_it(quartile, quartile_set, true);
    }
}

void IQRScorer::remove_even_quartile_it(quartile_t& quartile, 
    multiset<uint32_t>& quartile_set, bool less_than_quartile,
    bool is_first_it) {

    // handle quartile-sample placement
    if (is_first_it) {
        quartile = {
            double(*prev(quartile.it_f) + *next(quartile.it_f)) / 2.0,
            true,
            prev(quartile.it_f),
            next(quartile.it_f)
        };
    }
    else if (less_than_quartile) {
        quartile = {
            double(*(quartile.it_f) + *next(quartile.it_f)) / 2.0,
            true,
            quartile.it_f,
            next(quartile.it_f)
        };
    }
    else {
        quartile = {
            double(*prev(quartile.it_f) + *(quartile.it_f)) / 2.0,
            true,
            prev(quartile.it_f),
            quartile.it_f
        };
    }
}

void IQRScorer::remove_odd_quartile_it(quartile_t& quartile, 
    multiset<uint32_t>& quartile_set, bool less_than_quartile,
    bool is_first_it) {

    // handle quartile-sample placement
    if (is_first_it || less_than_quartile) {
        quartile = {
            double(*(quartile.it_s)),
            false,
            quartile.it_s,
            quartile_set.end()
        };
    }
    else {
        quartile = {
            double(*(quartile.it_f)),
            false,
            quartile.it_f,
            quartile_set.end()
        };
    }
}

void IQRScorer::remove_quartile(uint32_t sample, quartile_t& quartile,
    multiset<uint32_t>& quartile_set) {

    // find sample iterator
    auto it = quartile_set.find(sample);
    bool is_first_it = it == quartile.it_f;

    // update specified set quartile 
    if ((quartile_set.size() - 1) % 2) {
        if (double(sample) >= *(quartile.it_f)) 
            this->remove_odd_quartile_it(quartile, quartile_set, false, is_first_it);
        else this->remove_odd_quartile_it(quartile, quartile_set, true, is_first_it);
    }
    else {
        if (double(sample) >= *(quartile.it_f)) 
            this->remove_even_quartile_it(quartile, quartile_set, false, is_first_it);
        else this->remove_even_quartile_it(quartile, quartile_set, true, is_first_it);
    }

    // erase sample
    quartile_set.erase(it);
}

void IQRScorer::add_sample(uint32_t sample) {

    // initiate scorer
    if (this->sample_count == 0) {
        this->q1 = { double(sample), false, this->first_quartile_set.begin(), 
            this->first_quartile_set.end() };
        this->median = { double(sample), false, sample, 0 };
        this->q3 = { double(sample), false, this->third_quartile_set.begin(), 
            this->third_quartile_set.end() };
    }

    // handle single-sample case
    else if (this->sample_count == 1) {
        uint32_t val_f = sample < this->median.value_f ? sample : this->median.value_f;
        uint32_t val_s = sample < this->median.value_f ? this->median.value_f : sample;
        auto it_f = this->first_quartile_set.insert(val_f);
        auto it_s = this->third_quartile_set.insert(val_s);

        // update quartiles
        this->q1 = { double(val_f), false, it_f, this->first_quartile_set.end() };
        this->median = { double(val_f + val_s) / 2.0, true, val_f, val_s };
        this->q3 = { double(val_s), false, it_s, this->third_quartile_set.end() };
    }

    // handle variable-sample case
    else {
        if (this->median.is_composite_quartile) {
            if (sample < this->median.value_f) {
                this->add_quartile(sample, this->q1, this->first_quartile_set);
                uint32_t balance_sample = *(this->first_quartile_set.rbegin());
                this->median = { double(balance_sample), false, balance_sample, 0 };
                this->remove_quartile(balance_sample, this->q1, this->first_quartile_set);
            }
            else if (sample >= this->median.value_s) {
                this->add_quartile(sample, this->q3, this->third_quartile_set);
                uint32_t balance_sample = *(--this->third_quartile_set.rend());
                this->median = { double(balance_sample), false, balance_sample, 0 };
                this->remove_quartile(balance_sample, this->q3, this->third_quartile_set);
            }
            else this->median = { double(sample), false, sample, 0 };
        }
        else {
            if (sample >= this->median.value_f) {
                this->add_quartile(sample, q3, this->third_quartile_set);
                uint32_t balance_sample = *(--this->third_quartile_set.rend());
                uint32_t balance_median = this->median.value_f;
                this->median = { 
                    (double(balance_median) + double(balance_sample)) / 2.0,
                    true, balance_median, balance_sample
                };
                this->add_quartile(balance_median, q1, this->first_quartile_set);
            }
            else {
                this->add_quartile(sample, q1, this->first_quartile_set);
                uint32_t balance_sample = *(this->first_quartile_set.rbegin());
                uint32_t balance_median = this->median.value_f;
                this->median = { 
                    (double(balance_sample) + double(balance_median)) / 2.0,
                    true, balance_sample, balance_median
                };
                this->add_quartile(balance_median, q3, this->third_quartile_set);
            }
        }
    }

    // increment counters
    this->sample_sum += sample;
    this->sample_count++;
}

void IQRScorer::remove_sample(uint32_t sample) {}

double IQRScorer::get_iqr() { return this->get_q3() - this->get_q1(); }
double IQRScorer::get_q1() { return (this->q1).value; }
double IQRScorer::get_median() { return (this->median).value; }
double IQRScorer::get_q3() { return (this->q3).value; }

double IQRScorer::get_mean() {
    return double(this->sample_sum) / double(this->sample_count); 
}

bool IQRScorer::is_outlier() {
    return false;
}

uint32_t IQRScorer::fetch_random_sample() {
    return 0;
}

void IQRScorer::print_quartile_set() {

    // print first quartile
    auto curr_it = this->first_quartile_set.begin();
    cout << *curr_it;
    while (++curr_it != this->first_quartile_set.end())
        cout << " - " << *curr_it;

    // print median
    if (!this->median.is_composite_quartile) 
        cout << " - " << this->median.value_f;

    // print third quartile
    curr_it = this->third_quartile_set.begin();
    while (curr_it != this->third_quartile_set.end())
        cout << " - " << *curr_it++;
    cout << endl << flush;
}   