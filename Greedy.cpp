#include "Greedy.h"

Greedy::Greedy(const Dataset &d) : Algorithm(d)
{
}

Greedy::~Greedy()
{
}

double Greedy::get_solutions(Seed &seed)
{
    auto const num_nodes = data.get_data_size();
    seed.clear();
    seed = Seed(Constants::K);

    double current_obj = query(seed);
    vector<bool> in_seed(num_nodes, false);

    spdlog::info("Start GREEDY: current obj {}", current_obj);

    set<uint> I;
    for (auto i = 0; i < Constants::K; ++i)
    {
        I.emplace(i);
    }

    while (!I.empty())
    {
        uint max_e = num_nodes, max_k = Constants::K;
        double max_obj = 0.0;
        for (auto const &k : I)
        {
#pragma omp parallel for
            for (auto e = 0; e < num_nodes; ++e)
            {
                if (in_seed[e])
                    continue;
                auto seed_tmp = seed;
                seed_tmp[k].emplace_back(e);
                auto tmp_obj = query(seed_tmp);
                if (tmp_obj > max_obj)
                {
#pragma omp critical
                    {
                        if (tmp_obj > max_obj)
                        {
                            max_obj = tmp_obj;
                            max_e = e;
                            max_k = k;
                        }
                    }
                }
            }
        }

        if (max_obj <= current_obj)
        {
            spdlog::info("Stop GREEDY since no increment in obj");
            break;
        }

        if (max_e == num_nodes || max_k == Constants::K)
        {
            spdlog::info("Something wrong happened");
            throw "error here";
        }

        seed[max_k].emplace_back(max_e);
        in_seed[max_e] = true;
        current_obj = max_obj;
        spdlog::info("Add e {} to k {}, current obj {}", max_e, max_k, current_obj);

        if (seed[max_k].size() < Constants::B)
            continue;

        spdlog::info("k {} is full", max_k);
        I.erase(max_k);

        if (I.size() > 1 || is_obj_monotone() || I.empty())
            continue;

        // reach this point if I.size() == 1 and obj is non monotone
        auto const k = *I.begin();
        non_monotone_submodular_alg(seed, k, in_seed, current_obj);
        break;
    }

    spdlog::info("End GREEDY: obj {}", current_obj);

    return current_obj;
}