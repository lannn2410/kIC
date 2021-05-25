#pragma once
#include "Algorithm.h"

class SimpleStream : public Algorithm
{
public:
    SimpleStream(const Dataset &d);
    ~SimpleStream();
    double get_solutions(Seed &seed);
};