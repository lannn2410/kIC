#include "MpCommon.h"
#include <algorithm>
#include <numeric>
#include "Constants.h"

MpCommon *MpCommon::instance = nullptr;

MpCommon::MpCommon()
{
	seed = new int[10000];
	for (int i = 0; i < 10000; i++)
	{
		seed[i] = rand();
	}
}

MpCommon *MpCommon::getInstance()
{
	if (!instance)
		instance = new MpCommon();
	return instance;
}

unsigned MpCommon::randomInThread(int thread_id)
{
	unsigned tmp = seed[thread_id % 10000];
	tmp = tmp * 17931 + 7391;
	seed[thread_id % 10000] = tmp;
	return tmp;
}

set<uint> MpCommon::random_k_select(set<uint> S, const uint &k)
{
	if (k >= S.size())
	{
		return S;
	}

	set<uint> re{};
	for (int i = 0; i < k; ++i)
	{
		auto r = randomInThread(omp_get_thread_num()) % S.size();
		auto it = std::begin(S);
		std::advance(it, r);
		re.emplace(*it);
		S.erase(*it);
	}
	return re;
}

uint MpCommon::get_element_by_index_from_set(const uint &index, const set<uint> S) const
{
	auto it = std::begin(S);
	std::advance(it, index);
	return *it;
}

uint MpCommon::weighted_select(const vector<double> &w, const uint &ex)
{
	const uint max_idx = std::max_element(w.begin(), w.end()) - w.begin();
	vector<double> normalized_w;
	for (auto i = 0; i < w.size(); ++i)
	{
		if (i == max_idx)
			normalized_w.emplace_back(1.0);
		else
			normalized_w.emplace_back(pow(w[i] / w[max_idx], ex));
	}

	double sum_w = accumulate(normalized_w.begin(), normalized_w.end(), 0.0);
	auto r = ((double)(randomInThread(omp_get_thread_num()) % 10000)) / 10000.0;
	r *= sum_w;
	double acc = 0.0;
	for (int i = 0; i < normalized_w.size(); ++i)
	{
		if (acc <= r && acc + normalized_w[i] >= r)
		{
			return i;
		}
		acc += normalized_w[i];
	}
	return 0;
}