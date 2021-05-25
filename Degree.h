#pragma once
#include "Algorithm.h"

class Degree : public Algorithm
{
public:
    Degree(const Dataset &d);
    ~Degree();
    double get_solutions(Seed &seed);
};