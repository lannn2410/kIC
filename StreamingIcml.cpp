#include "StreamingIcml.h"
#include <algorithm>
#include <math.h>

StreamingIcml::StreamingIcml(const Dataset &d) : Algorithm(d)
{
}

StreamingIcml::~StreamingIcml()
{
}

double StreamingIcml::get_solutions(Seed &seeds)
{
    clear();
    auto const num_nodes = data.get_data_size();
    double max_gain = 0.0;
    int j_min, j_max = 0;
    double const log_delta = log(1.0 + Constants::ICML_DELTA);
    spdlog::info("ICML log delta {}", log_delta);
    for (auto e = 0; e < num_nodes; ++e)
    {
        spdlog::info("e {} max gain {} max obj {}", e, max_gain, max_solution);

        bool change_max_gain = false;
        // get max gain of a single e,i
        // #pragma omp parallel
        for (int i = 0; i < Constants::K; ++i)
        {
            Seed single_seed(Constants::K);
            single_seed[i].emplace_back(e);
            double const tmp = query(single_seed);
            if (max_gain >= tmp)
                continue;
            // #pragma omp critical
            //             {
            //                 if (tmp > max_gain)
            //                 {
            max_gain = tmp;
            change_max_gain = true;
            //     }
            // }
        }

        if (change_max_gain)
        {
            int const j_tmp = ceil(log(max_gain / (Constants::B * Constants::K * Constants::ICML_M)) / log_delta); // could < 0
            j_min = j_tmp >= 0 ? j_tmp : 0;
            j_max = floor(log(max_gain) / log_delta);
            spdlog::info("Max gain change: max gain {} down {} up {}", max_gain, j_min, j_max);
            if (j_max + 1 <= sub_seeds.size())
                continue;

            sub_seeds.resize(j_max + 1, Seed(Constants::K));
            influences.resize(j_max + 1, 0.0);
            for (int j = thresholds.size(); j < j_max + 1; ++j)
            {
                thresholds.push_back(pow(1.0 + Constants::ICML_DELTA, j));
            }
        }

        // select element for each sub seed
        if (sub_seeds.size() > 0)
        {
            // #pragma omp parallel for
            for (int j = j_min; j <= j_max; ++j)
            {
                int const i = select_k(j, e);
                if (i < 0)
                {
                    // spdlog::info("No k eligible for e {} in threshold {}", e, j);
                    continue;
                }

                spdlog::info("Put e {} to k {} in threshold {}", e, i, j);
                sub_seeds[j][i].emplace_back(e);
            }
        }
    }
    spdlog::info("End Icml Streaming, obj {}", max_solution);
    if (max_solution_index >= 0 && max_solution_index < sub_seeds.size())
    {
        seeds = sub_seeds[max_solution_index];
    }
    return max_solution;
}

void StreamingIcml::clear()
{
    max_solution = 0.0;
    max_solution_index = -1;
    sub_seeds.clear();
    thresholds.clear();
    influences.clear();
}
