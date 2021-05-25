#include "Streaming.h"

Streaming::Streaming(const Dataset &d) : Algorithm(d)
{
}

Streaming::~Streaming()
{
}

double Streaming::get_solutions(Seed &seed)
{
    auto const num_nodes = data.get_data_size();
    seed.clear();
    seed = Seed(Constants::K);

    // store weights of elements in seed
    vector<vector<double>> weights(Constants::K);

    double current_obj = query(seed);

    spdlog::info("Start STREAMING: current obj {}", current_obj);
    for (auto i = 0; i < num_nodes; ++i)
    {
        spdlog::info("Receive e {}", i);

        double r = common_ins->randomInThread(omp_get_thread_num()) % 1000;
        r /= 1000;
        if (r > Constants::STREAMING_DELTA)
        {
            spdlog::info("e {} not considered due to prob. selection", i);
            continue;
        }

        vector<double> w_k(Constants::K, -1.0); // weight of the element in each k
        vector<uint> eliminate_idx(Constants::K, num_nodes);
        vector<double> new_obj(Constants::K, 0);
        vector<double> inc_weight(Constants::K, 0);

// #pragma omp parallel for
        for (auto k = 0; k < Constants::K; ++k)
        {

            spdlog::info("Working e {} k {}", i, k);
            if (seed[k].size() < Constants::B)
            {
                spdlog::info("Start e {} k {} - budget not full, b {}", i, k, seed[k].size());
                auto seed_tmp = seed;
                seed_tmp[k].emplace_back(i);
                new_obj[k] = query(seed_tmp);
                w_k[k] = new_obj[k] - current_obj;
                if (w_k[k] > 0)
                {
                    spdlog::info("k {} is eligible for e {} - gain {} > 0", k, i, w_k[k]);
                    inc_weight[k] = w_k[k];
                }
                else
                {
                    spdlog::info("k {} is not eligible for e {} - gain {} <= 0", k, i, w_k[k]);
                }
            }
            else
            {
                spdlog::info("Start e {} k {} - budget full, b {}", i, k, seed[k].size());
                int const minIdx = std::min_element(weights.begin(), weights.end()) - weights.begin();
                spdlog::info("Potential remove {} to put e {} in k {}", seed[k][minIdx], i, k);
                auto seed_tmp = seed;
                seed_tmp[k].erase(seed_tmp[k].begin() + minIdx);
                auto const obj_tmp = query(seed_tmp);
                seed_tmp[k].emplace_back(i);
                new_obj[k] = query(seed_tmp);
                w_k[k] = new_obj[k] - obj_tmp;
                spdlog::info("Weight {} new obj {} obj tmp {}", w_k[k], new_obj[k], obj_tmp);
                if (w_k[k] >= (1 + Constants::STREAMING_ALPHA) * weights[k][minIdx])
                {
                    spdlog::info("k {} is eligible for e {}", k, i);
                    eliminate_idx[k] = minIdx;
                    inc_weight[k] = w_k[k] - weights[k][minIdx];
                }
                else
                {
                    spdlog::info("k {} is not eligible for e {} - weight {} < (1 + {}) * penalty {}",
                                 k, i, w_k[k], Constants::STREAMING_ALPHA, weights[k][minIdx]);
                }
            }
        }

        int const sel_k = std::max_element(inc_weight.begin(), inc_weight.end()) - inc_weight.begin();
        if (inc_weight[sel_k] < 1e-20)
        {
            spdlog::info("Ignore since no increment in weight if adding e {}", i);
            continue;
        }

        if (eliminate_idx[sel_k] < num_nodes)
        {
            spdlog::info("Remove e {} out of k {}", seed[sel_k][eliminate_idx[sel_k]], sel_k);
            seed[sel_k].erase(seed[sel_k].begin() + eliminate_idx[sel_k]);
            weights[sel_k].erase(weights[sel_k].begin() + eliminate_idx[sel_k]);
        }
        seed[sel_k].emplace_back(i);
        weights[sel_k].emplace_back(w_k[sel_k]);
        current_obj = new_obj[sel_k];
        spdlog::info("e {} is put into k {}, current obj {}", i, sel_k, current_obj);
    }
    spdlog::info("End STREAMING: obj {}", current_obj);
    return current_obj;
}