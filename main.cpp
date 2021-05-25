#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip> // setprecision
#include <string>
#include <sstream>
#include <numeric>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <tuple>
#define PORT 8080

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "Constants.h"
#include "MpCommon.h"
#include "Dataset.h"

#include "Streaming.h"
#include "Streaming2.h"
#include "Greedy.h"
#include "Deterministic.h"
#include "Randomized.h"
#include "SingleGreedy.h"
#include "StreamGreedy.h"
#include "Degree.h"
#include "SimpleStream.h"
#include "PureGreedy.h"

using namespace std;

enum CHANGE
{
	B = 0,
	DELTA_ALPHA = 1
};
enum ALG
{
	STREAMING = 0,
	STREAMING2 = 1,
	GREEDY = 2,
	PUREGREEDY = 3,
	STREAMGREEDY = 4,
	DEGREE = 5,
	DSTREAM = 6,
	RSTREAM = 7,
	SIMPLESTREAM = 8,
	SINGLE = 9
};

static const char *AppString[] = {"SOCIAL", "MAXCUT"};
static const char *AlgString[] = {
	"STREAMING",
	"STREAMING2",
	"GREEDY",
	"PUREGREEDY",
	"STREAMGREEDY",
	"DEGREE",
	"DSTREAM",
	"RSTREAM",
	"SIMPLESTREAM",
	"SINGLE"};
static const char *ChangeString[] = {"BUDGET", "DELTA_ALPHA"};

struct Result
{
	// can run many times for one algorithms
	ALG algorithm;
	vector<double> obj{};
	vector<uint> violate_count{};
	vector<uint> num_queries{};
	vector<uint64_t> runtime{};
};

uint64_t timeMillisec()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(
			   system_clock::now().time_since_epoch())
		.count();
}

bool need_refine(const ALG &alg_kind)
{
	if (alg_kind == ALG::DEGREE || alg_kind == ALG::DSTREAM ||
		alg_kind == ALG::RSTREAM || alg_kind == ALG::PUREGREEDY)
		return true;
	return false;
}

Result
getResultAlgorithm(const Dataset &d, const ALG &alg_kind,
				   uint num_repeat = 1 /*number of repeats*/)
{
	Result re;
	Algorithm *alg;

	if (true)
	{
		switch (alg_kind)
		{
		case ALG::STREAMING:
			alg = new Streaming(d);
			break;
		case ALG::STREAMING2:
			alg = new Streaming2(d);
			break;
		case ALG::GREEDY:
			alg = new Greedy(d);
			break;
		case ALG::PUREGREEDY:
			alg = new PureGreedy(d);
			break;
		case ALG::STREAMGREEDY:
			alg = new StreamGreedy(d);
			break;
		case ALG::DEGREE:
			alg = new Degree(d);
			break;
		case ALG::SINGLE:
			alg = new SingleGreedy(d);
			break;
		case ALG::RSTREAM:
			alg = new Randomized(d);
			break;
		case ALG::DSTREAM:
			alg = new Deterministic(d);
			break;
		case ALG::SIMPLESTREAM:
			alg = new SimpleStream(d);
			break;
		default:
			alg = new Streaming2(d);
			break;
		}
	}

	for (int i = 0; i < num_repeat; ++i)
	{
		Seed seeds(Constants::K);
		alg->reset_num_queries();

		// run the algorithm
		auto start = timeMillisec();
		auto obj = alg->get_solutions(seeds);
		uint violate_count = 0;
		if (need_refine(alg_kind))
		{
			tie(obj, violate_count) = alg->refine_seed(seeds);
		}
		auto num_queries = alg->get_num_queries();
		auto dur = timeMillisec() - start;

		spdlog::info("Done {} for {} times, obj {}, no. queries {}, runtime {}",
					 AlgString[(int)alg_kind], i, obj, num_queries, dur);

		// save to result
		re.algorithm = alg_kind;
		re.obj.emplace_back(obj);
		re.violate_count.emplace_back(violate_count);
		re.num_queries.emplace_back(num_queries);
		re.runtime.emplace_back(dur);
	}

	delete alg;
	return re;
}

void make_folder(const string &folder_path)
{
#if defined(_WIN32)
	mkdir(folder_path.c_str());
#else
	mkdir(folder_path.c_str(), 0777); // notice that 777 is different than 0777
#endif
}

int get_num_repeat(ALG const &alg)
{
	if ((alg == ALG::STREAMING || alg == ALG::STREAMING2) && Constants::STREAMING_DELTA > 1.0 - 1e-10)
	{
		return 1;
	}

	if (Constants::APPLICATION == APP_TYPE::Social)
	{
		if (alg == ALG::RSTREAM || alg == ALG::SIMPLESTREAM)
			return Constants::NUM_REPEAT;
		return 1;
	}

	// maxcut application
	if (alg == ALG::STREAMGREEDY || alg == ALG::DEGREE || alg == ALG::DSTREAM)
	{
		return 1;
	}

	return Constants::NUM_REPEAT;
}

vector<Result> runAlgorithms(const Dataset &data, const CHANGE &change)
{
	vector<Result> re;
	if (change == CHANGE::B)
	{
		spdlog::info("Budget {}", Constants::B);
		for (int i = (int)ALG::STREAMING; i <= (int)ALG::SINGLE; ++i)
		{
			ALG alg = static_cast<ALG>(i);
			re.emplace_back(
				getResultAlgorithm(data, alg, get_num_repeat(alg)));
		}
		return re;
	}

	// change is on delta and alpha. This is solely for STREAMING
	re.emplace_back(
		getResultAlgorithm(data, ALG::STREAMING2, get_num_repeat(ALG::STREAMING2)));
	return re;
}

void runExperiment(const APP_TYPE &application, const string &file_name,
				   const uint &num_nodes = 0 /*for social app only*/,
				   const CHANGE &change = CHANGE::B,
				   const uint &min = 1, const uint &max = 10, const uint &step = 1)
{
	Constants::APPLICATION = application;
	long folder_prefix = time(NULL);
	auto folder_suffix = change == CHANGE::DELTA_ALPHA ? to_string(Constants::B)
													   : (to_string(Constants::STREAMING_ALPHA) + "_" + to_string(Constants::STREAMING_DELTA));

	const string re_folder = "result/" + to_string(folder_prefix) +
							 "_" + file_name + "_" + AppString[(int)application] +
							 "_" + ChangeString[change] +
							 "_" + folder_suffix;

	spdlog::info("Run experiments with file {}, change {}",
				 file_name, ChangeString[change]);
	spdlog::info("Results will be stored in {}", re_folder);

	make_folder(re_folder);

	ofstream writefile_query(re_folder + "/query.csv");
	ofstream writefile_sol(re_folder + "/solution.csv");
	ofstream writefile_runtime(re_folder + "/runtime.csv");
	ofstream writefile_violate(re_folder + "/violate.csv");

	if (!writefile_query.is_open() || !writefile_sol.is_open() ||
		!writefile_runtime.is_open() || !writefile_violate.is_open())
		throw "files cannot be written";

	string header = "";
	if (change == CHANGE::B)
	{
		header += "B,";
		for (auto const &s : AlgString)
		{
			header = header + s + ",";
		}
	}
	else
	{
		header = "alpha,delta,obj,runtime,query";
	}

	// const string header = change == CHANGE::B ? "B,Streaming,Greedy,StreamGreedy,Single," : "obj,alpha,delta,";

	writefile_query << header << endl;
	writefile_sol << header << endl;
	writefile_runtime << header << endl;
	writefile_violate << header << endl;

	// lambda function to get average result
	auto getAverageResult = [](Result const &result) -> tuple<double, double, double, double> {
		if (result.obj.empty())
			throw "Error: No result";
		double obj = accumulate(result.obj.begin(), result.obj.end(), 0);
		obj /= result.obj.size();
		double violate = accumulate(result.violate_count.begin(), result.violate_count.end(), 0.0);
		violate /= result.violate_count.size();

		double query = accumulate(result.num_queries.begin(),
								  result.num_queries.end(), 0);
		query /= result.num_queries.size();
		double runtime = accumulate(result.runtime.begin(),
									result.runtime.end(), 0);
		runtime /= result.runtime.size();
		return make_tuple(obj, violate, query, runtime);
	};

	// lambda function to write results to files
	auto writeResultToFile = [&writefile_query, &writefile_sol, &writefile_runtime, &writefile_violate,
							  &change, &getAverageResult, &re_folder, &application, &file_name](vector<Result> const &results) {
		if (change == CHANGE::B)
		{
			writefile_query << Constants::B << ",";
			writefile_sol << Constants::B << ",";
			writefile_runtime << Constants::B << ",";
			writefile_violate << Constants::B << ",";

			for (auto const &result : results)
			{
				double obj, violate, query, runtime;
				tie(obj, violate, query, runtime) = getAverageResult(result);
				writefile_sol << obj << ",";
				writefile_query << query << ",";
				writefile_runtime << runtime << ",";
				writefile_violate << violate << ",";
			}
			writefile_query << endl;
			writefile_sol << endl;
			writefile_runtime << endl;
			writefile_violate << endl;
			return;
		}

		// for change alpha delta, which solely runs Streaming
		if (results.size() > 1 || results.empty())
			throw "Error: should have only 1 result";
		auto const &result = results[0];
		double obj, violate, query, runtime;
		tie(obj, violate, query, runtime) = getAverageResult(result);
		writefile_sol << Constants::STREAMING_ALPHA << ","
					  << Constants::STREAMING_DELTA << ","
					  << obj << "," << runtime << "," << query << endl;
	};

	Dataset data;
	data.read_network(num_nodes, "data/" + file_name);

	if (change == CHANGE::B)
	{
		for (Constants::B = min;
			 Constants::B <= max;
			 Constants::B += step)
		{

			writeResultToFile(runAlgorithms(data, change));
		}
	}
	else
	{
		for (Constants::STREAMING_ALPHA = 0.0;
			 Constants::STREAMING_ALPHA <= 2.01;
			 Constants::STREAMING_ALPHA += 0.1)
		{
			for (Constants::STREAMING_DELTA = 0.1;
				 Constants::STREAMING_DELTA <= 1.01;
				 Constants::STREAMING_DELTA += 0.1)
			{
				writeResultToFile(runAlgorithms(data, change));
			}
		}
	}
	writefile_query.close();
	writefile_sol.close();
	writefile_runtime.close();
}

void print_help()
{
	cout << "Options: " << endl;
	cout << "-h <print help>" << endl
		 << "-t <application type, 0: IM, 1: MaxCut> # default: 0" << endl
		 << "-a <algorithm type, 0: streaming, 1: greedy, 2: sc-greedy, 3: streamgreedy, 4: degree, 5: dstream, 6: rstream, 7:simplestream, 8: single> # default: 1" << endl
		 << "-b <individual budget> # default: 10" << endl
		 << "-k <in k-submodular> # default: 3" << endl
		 << "-alpha <streaming alpha> # default: 0.1" << endl
		 << "-delta <streaming delta> # default: 1.0" << endl
		 << "-p <number of threads> # default: 10";
}

ALG getAlgType(int a)
{
	switch (a)
	{
	case 0:
		return ALG::STREAMING2;
		break;
	case 1:
		return ALG::GREEDY;
		break;
	case 2:
		return ALG::PUREGREEDY;
		break;
	case 3:
		return ALG::STREAMGREEDY;
		break;
	case 4:
		return ALG::DEGREE;
		break;
	case 5:
		return ALG::DSTREAM;
		break;
	case 6:
		return ALG::RSTREAM;
		break;
	case 7:
		return ALG::SIMPLESTREAM;
		break;
	case 8:
		return ALG::SINGLE;
		break;
	default:
		return ALG::GREEDY;
		break;
	}
}

struct Args
{
	APP_TYPE type = APP_TYPE::Social;
	ALG alg = ALG::GREEDY;
	int b = 10, k = 3, p = 10;		 // individual budget, no. group - budget per group = b / k
	double alpha = 0.1, delta = 1.0; // alpha, delta for streaming
	bool is_help = false;
};

Args parseArgs(int argc, char **argv)
{
	Args re;
	int i = 1;
	while (i <= argc - 1)
	{
		string arg = argv[i];
		if (arg == "-h")
		{
			re.is_help = true;
			break;
		}
		if (i + 1 >= argc)
			break;
		string s_val = argv[i + 1];
		if (s_val == "-h")
		{
			re.is_help = true;
			break;
		}
		std::string::size_type sz;
		if (arg == "-t" || arg == "-a" || arg == "-b" ||
			arg == "-k" || arg == "-p")
		{
			int val = std::stoi(s_val, &sz);
			if (arg == "-t")
			{
				if (val < 0 || val > 1)
				{
					cout << "Application is not valid" << endl;
					re.is_help = true;
					break;
				}
				re.type = val == 0 ? APP_TYPE::Social : APP_TYPE::Maxcut;
			}
			else if (arg == "-a")
			{
				if (val < 0 || val > 4)
				{
					cout << "Alg is not valid" << endl;
					re.is_help = true;
					break;
				}
				re.alg = getAlgType(val);
			}
			else if (arg == "-b")
			{
				if (val < 1)
				{
					cout << "b < 1" << endl;
					re.is_help = true;
					break;
				}
				re.b = val;
			}
			else if (arg == "-k")
			{
				if (val < 1)
				{
					cout << "k < 1" << endl;
					re.is_help = true;
					break;
				}
				re.k = val;
			}
			else if (arg == "-p")
			{
				if (val < 1)
				{
					cout << "Num. threads is not valid" << endl;
					re.is_help = true;
					break;
				}
				re.p = val;
			}
		}
		else if (arg == "-alpha" || arg == "-delta")
		{
			double val = std::stod(s_val, &sz);
			if (arg == "-alpha")
			{
				if (val < 0)
				{
					cout << "alpha has to be greater than 0" << endl;
					re.is_help = true;
					break;
				}
				re.alpha = val;
			}
			else if (arg == "-delta")
			{
				if (val < 0 || val > 1)
				{
					cout << "Delta has to be in range [0,1]" << endl;
					re.is_help = true;
					break;
				}
				re.delta = val;;
			}
		}
		else
		{
			cout << "Wrong arguments" << endl;
			re.is_help = true;
			break;
		}

		i += 2;
	}

	return re;
}

void run_command(const Args &r)
{
	if (r.is_help)
	{
		print_help();
		return;
	}
	Constants::APPLICATION = r.type;
	Constants::B = r.b;
	Constants::K = r.k;
	Constants::STREAMING_ALPHA = r.alpha;
	Constants::STREAMING_DELTA = r.delta;
	omp_set_num_threads(r.p);
	Dataset data;
	if (r.type == APP_TYPE::Social)
	{
		data.read_network(4039, "data/facebook_combined.txt"); // hard code
	}
	else
	{
		data.read_network(18772, "data/CA-AstroPh.txt");
	}
	

	Result res = getResultAlgorithm(data, r.alg);

	if (res.obj.empty())
	{
		cout << "The algorithm ran unsucessfully" << endl;
		return;
	}
	cout << "Objective: " << res.obj[0] << endl
		 << "Number of queries: " << res.num_queries[0] << endl;
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	run_command(parseArgs(argc, argv));
	// omp_set_num_threads(Constants::NUM_THREADS);

	// // Constants::B = 20;
	// // runExperiment(APP_TYPE::Maxcut, "CA-AstroPh.txt",
	// // 			  18772, CHANGE::DELTA_ALPHA);

	// Constants::STREAMING_ALPHA = 1.4;
	// Constants::STREAMING_DELTA = 1.0;
	// runExperiment(APP_TYPE::Social, "facebook_combined.txt",
	// 			  4039, CHANGE::B, 5, 50, 5);

	// Constants::STREAMING_ALPHA = 0.2;
	// Constants::STREAMING_DELTA = 1.0;
	// runExperiment(APP_TYPE::Maxcut, "CA-AstroPh.txt",
	// 			  18772, CHANGE::B, 5, 50, 5);

	// Constants::B = 20;
	// runExperiment(APP_TYPE::Social, "facebook_combined.txt",
	// 			  4039, CHANGE::DELTA_ALPHA);

	return 1;
}