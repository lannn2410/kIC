#include "StreamGreedy.h"

StreamGreedy::StreamGreedy(const Dataset &d) : Algorithm(d)
{
}

StreamGreedy::~StreamGreedy()
{
}

double StreamGreedy::get_solutions(Seed &seed)
{
    auto const num_nodes = data.get_data_size();
    seed.clear();
    seed = Seed(Constants::K);

    double current_obj = query(seed);

    spdlog::info("Start STREAM GREEDY: current obj {}", current_obj);
    for (auto e = 0; e < num_nodes; ++e)
    {
        uint k_occupied = Constants::K; // value of k that this e already belong
        spdlog::info("Consider e {}", e);
        for (auto k = 0; k < Constants::K; ++k)
        {
            auto seed_tmp = seed;
            if (k_occupied < Constants::K)
            {
                // if this e already occupied, it should be the last elements on that k
                seed_tmp[k_occupied].pop_back();
                spdlog::info("e {} is occupied by k {}, temporarily pop", e, k_occupied);
            }

            seed_tmp[k].emplace_back(e);
            if (seed_tmp[k].size() <= Constants::B)
            {
                auto tmp_obj = query(seed_tmp);
                if (tmp_obj > current_obj)
                {
                    seed = seed_tmp;
                    current_obj = tmp_obj;
                    k_occupied = k;
                    spdlog::info("Obj {} increase by adding e {} to k {}", current_obj, e, k);
                }
                continue;
            }

            if (seed_tmp[k].size() > Constants::B) // size = B + 1
            {
                // remove one element
                for (int i = 0; i < seed_tmp[k].size() - 1; ++i)
                {
                    auto seed_tmp_2 = seed_tmp;
                    seed_tmp_2[k].erase(seed_tmp_2[k].begin() + i);
                    auto tmp_obj = query(seed_tmp_2);
                    if (tmp_obj <= current_obj)
                        continue;

                    seed = seed_tmp_2;
                    current_obj = tmp_obj;
                    k_occupied = k;
                    spdlog::info("Obj {} increase by replace e {} by e {} in k {}", current_obj, i, e, k);
                    break;
                }
            }
        }
    }
    spdlog::info("End STREAM GREEDY: obj {}", current_obj);
    return current_obj;
}