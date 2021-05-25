#pragma once
#include "Algorithm.h"

class StreamGreedy : public Algorithm
{
public:
    StreamGreedy(const Dataset &d);
    ~StreamGreedy();
    double get_solutions(Seed &seed);
};