#include "Deterministic.h"

Deterministic::Deterministic(const Dataset &d) : StreamingIcml(d)
{
}

Deterministic::~Deterministic()
{
}

int Deterministic::select_k(int j, uint e)
{
	double max = 0.0;
	uint jj = -1;
	uint seed_size = 0;
	for (int ii = 0; ii < Constants::K; ++ii)
	{
		seed_size += sub_seeds[j][ii].size();
	}

	// if budget has been reached, return
	if (seed_size >= Constants::K * Constants::B)
		return -1;

	for (int ii = 0; ii < Constants::K; ++ii)
	{
		auto seed_tmp = sub_seeds[j];
		seed_tmp[ii].emplace_back(e);
		double const tmp_obj = query(seed_tmp);
		if (max < tmp_obj)
		{
			max = tmp_obj;
			jj = ii;
		}
	}

	if (max >= (seed_size + 1) * thresholds[j])
	{
		if (max > max_solution)
		{
			max_solution = max;
			max_solution_index = j;
		}
		return jj;
	}
	else
		return -1;
}
