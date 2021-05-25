#pragma once
#include "Dataset.h"
#include "Constants.h"
#include "MpCommon.h"

using namespace std;

class Algorithm
{
public:
    Algorithm(const Dataset &d);
    virtual double get_solutions(Seed &seeds) = 0; // seeds returned in form of 0-1 vector
    uint get_num_queries() const;
    void reset_num_queries();
    bool is_obj_monotone() const;
    tuple<double, uint> refine_seed(const Seed &seed);

protected:
    void non_monotone_submodular_alg(Seed &seed, uint const &k,
                                     vector<bool> &in_seed, double &current_obj);
    double query(const Seed &s);
    const Dataset &data;
    MpCommon *common_ins;
    uint num_queries;
};
