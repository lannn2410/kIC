#pragma once
#include "Algorithm.h"
 

class StreamingIcml : public Algorithm
{
public:
	StreamingIcml(const Dataset &d);
	~StreamingIcml();
	double get_solutions(Seed &seeds);
	virtual int select_k(int j, uint e) = 0; // select element, different in each streaming algorithm
protected:
    void clear();
	double max_solution; // save the influence of the best among sub seeds
	int max_solution_index; // save the index of best solution
	
	vector<Seed> sub_seeds; 
	// store seeds of each (1+delta)^j, index of first list is j
	
	vector<double> thresholds; // store value of (1+\delta)^j
	vector<double> influences; // store estimated influence of each (1+\delta)^j
};

