#pragma once
#include "StreamingIcml.h"

class Randomized : public StreamingIcml
{
public:
	Randomized(const Dataset &d);
	~Randomized();
	int select_k(int j, uint e);
};

