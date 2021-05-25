#pragma once
#include "Algorithm.h"

class PureGreedy : public Algorithm
{
public:
    PureGreedy(const Dataset &d);
    ~PureGreedy();
    double get_solutions(Seed &seed);
};