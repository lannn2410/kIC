#pragma once
#include "StreamingIcml.h"

class Deterministic : public StreamingIcml
{
public:
	Deterministic(const Dataset &d);
	~Deterministic();
	int select_k(int j, uint e);
};

