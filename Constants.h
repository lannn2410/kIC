#pragma once
#include <string>

using namespace std;

enum class APP_TYPE {Social, Maxcut};

class Constants
{
public:
	static const int NUM_THREADS;
	static const int NUM_SAMPLES;
    static APP_TYPE APPLICATION;
    static int K; // k-submodular
    static int B; // budget per k
    
    static const int NUM_REPEAT; // no. repeat run of a randomize alg

    // for streaming
    static double STREAMING_ALPHA;
    static double STREAMING_DELTA;

    // for ICML streaming
    static const double ICML_DELTA;
    static const int ICML_M;
};

