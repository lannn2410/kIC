#pragma once
#include "Algorithm.h"

class SingleGreedy : public Algorithm
{
public:
    SingleGreedy(const Dataset &d);
    ~SingleGreedy();
    double get_solutions(Seed &seed);
};