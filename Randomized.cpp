#include "Randomized.h"
#include <stdlib.h>
#include <math.h>

Randomized::Randomized(const Dataset &d) : StreamingIcml(d)
{
}

Randomized::~Randomized()
{
}

int Randomized::select_k(int j, uint e)
{
	vector<double> inf(Constants::K), d(Constants::K, 0.0);
	uint T = 0;

	// spdlog::info("Randomized streaming: e {} threshold {}", e, thresholds[j]);

	uint seed_size = 0;
	for (int ii = 0; ii < Constants::K; ++ii)
	{
		seed_size += sub_seeds[j][ii].size();
	}

	// if budget has been reached, return
	if (seed_size >= Constants::K * Constants::B)
		return -1;

	// #pragma omp parallel for
	for (int i = 0; i < Constants::K; ++i)
	{
		auto seed_tmp = sub_seeds[j];
		seed_tmp[i].emplace_back(e);
		inf[i] = query(seed_tmp);

		d[i] = inf[i] - influences[j];
		d[i] = d[i] >= thresholds[j] ? d[i] : 0;
		if (d[i] > 0)
		{
			spdlog::info("Randomize e {} k {} d {}", e, i, d[i]);
			// throw "temp";
			// #pragma omp critical
			// 			{
			++T;
			// }
		}
	}

	if (T == 0)
		return -1;
	else if (T == 1)
	{
		for (int i = 0; i < Constants::K; ++i)
		{
			if (d[i] > 0)
			{
				influences[j] = inf[i];
				if (influences[j] > max_solution)
				{
					// #pragma omp critical
					// 					{
					// 						if (influences[j] > max_solution)
					// 						{
					max_solution = influences[j];
					max_solution_index = j;
					// 	}
					// }
				}
				return i;
			}
		}
	}
	else
	{
		double D = 0.0;
		int const maxIdx = std::max_element(d.begin(), d.end()) - d.begin();
		for (int i = 0; i < Constants::K; ++i)
		{
			if (i == maxIdx)
				d[i] = 1.0;
			else
				d[i] = pow(d[i] / d[maxIdx], T - 1);
			D += d[i];
		}
		double sum = 0.0;
		double r = (double)(common_ins->randomInThread(omp_get_thread_num()) % 10000) / 10000.0 * D;
		for (int i = 0; i < Constants::K; ++i)
		{
			if (sum <= r && r < sum + d[i] && sub_seeds[j][i].size() < Constants::B)
			{
				influences[j] = inf[i];
				if (influences[j] > max_solution)
				{
					// #pragma omp critical
					// 					{
					// 						if (influences[j] > max_solution)
					// 						{
					max_solution = influences[j];
					max_solution_index = j;
					// 	}
					// }
				}
				return i;
			}
			sum += d[i];
		}
	}

	return -1;
}
