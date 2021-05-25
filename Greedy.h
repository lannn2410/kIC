#pragma once
#include "Algorithm.h"

class Greedy : public Algorithm
{
public:
    Greedy(const Dataset &d);
    ~Greedy();
    double get_solutions(Seed &seed);
};