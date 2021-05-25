#pragma once
#include "Algorithm.h"

class Streaming2 : public Algorithm
{
public:
    Streaming2(const Dataset &d);
    ~Streaming2();
    double get_solutions(Seed &seed);
};