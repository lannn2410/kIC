#pragma once
#include "Algorithm.h"

class Streaming : public Algorithm
{
public:
    Streaming(const Dataset &d);
    ~Streaming();
    double get_solutions(Seed &seed);
};