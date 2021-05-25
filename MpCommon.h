#pragma once
#include <omp.h>
#include <vector>
#include <string>
#include <set>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

using namespace std;
typedef unsigned int uint;

class MpCommon
{
public:
	MpCommon();
	static MpCommon *getInstance();
	unsigned randomInThread(int thread_id);
	set<uint> random_k_select(set<uint> S, const uint &k);
	uint weighted_select(const vector<double> &w, const uint& ex = 1);

	uint get_element_by_index_from_set(const uint& index, const set<uint> S) const;

private:
	static MpCommon *instance;
	int *seed;
};
