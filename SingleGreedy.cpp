#include "SingleGreedy.h"

SingleGreedy::SingleGreedy(const Dataset &d) : Algorithm(d)
{
}

SingleGreedy::~SingleGreedy()
{
}

double SingleGreedy::get_solutions(Seed &seed)
{
    auto const num_nodes = data.get_data_size();
    seed.clear();
    seed = Seed(Constants::K);

    spdlog::info("Start SINGLE GREEDY");

    vector<Seed> seed_k(Constants::K, Seed(Constants::K));
    vector<vector<bool>> in_seed_k(Constants::K, vector<bool>(num_nodes, false));
    vector<vector<uint>> in_seed(num_nodes); // node -> list of k that it is in single solution
    for (auto k = 0; k < Constants::K; ++k)
    {
        double current_obj = query(seed);
        if (!is_obj_monotone())
        {
            non_monotone_submodular_alg(seed_k[k], k, in_seed_k[k], current_obj);
            for (auto const& e : seed_k[k][k])
            {
                in_seed[e].emplace_back(k);
            }
            continue;
        }

        uint max_e = num_nodes;
        double max_obj = current_obj;

        for (auto b = 0; b < Constants::B; ++b)
        {
            for (auto e = 0; e < num_nodes; ++e)
            {
                if (in_seed_k[k][e])
                    continue;
                auto seed_tmp = seed_k[k];
                seed_tmp[k].emplace_back(e);
                auto tmp_obj = query(seed_tmp);
                if (tmp_obj > max_obj)
                {
                    max_obj = tmp_obj;
                    max_e = e;
                }
            }

            if (max_obj <= current_obj)
            {
                spdlog::info("Stop GREEDY since no increment in obj");
                break;
            }

            if (max_e == num_nodes)
            {
                spdlog::info("Something wrong happened");
                throw "error here";
            }

            seed_k[k][k].emplace_back(max_e);
            in_seed_k[k][max_e] = true;
            in_seed[max_e].emplace_back(k);
            current_obj = max_obj;
            spdlog::info("Add e {} to k {}, current obj {}", max_e, k, current_obj);
        }
    }

    // merge all single solution, random choose a k if e appear more than 1 k
    for (auto i=0; i<num_nodes; ++i)
    {
        if (in_seed[i].empty()) continue;
        auto r = rand() % in_seed[i].size();
        auto k = in_seed[i][r];
        seed[k].emplace_back(i);
    }

    auto current_obj = query(seed);

    spdlog::info("End SINGLE GREEDY: obj {}", current_obj);

    return current_obj;
}