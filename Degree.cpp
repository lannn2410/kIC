#include "Degree.h"

Degree::Degree(const Dataset &d) : Algorithm(d)
{
}

Degree::~Degree()
{
}

double Degree::get_solutions(Seed &seed)
{
    auto const num_nodes = data.get_data_size();
    seed.clear();
    seed = Seed(Constants::K);

    spdlog::info("Start DEGREE");

    auto const node_degrees = data.get_node_degrees();
    vector<int> node_idx(num_nodes, 0);
    for (int i = 0; i < num_nodes; ++i)
    {
        node_idx[i] = i;
    }

    // sort elements in descending order w.r.t degree
    sort(node_idx.begin(), node_idx.end(), [&node_degrees](size_t i1, size_t i2) {
        return node_degrees[i1] > node_degrees[i2];
    });

    for (auto i = 0; i < Constants::B * Constants::K; ++i)
    {
        double max_obj = 0.0;
        uint sel_k = Constants::K;
#pragma omp parallel for
        for (auto k = 0; k < Constants::K; ++k)
        {
            auto seed_tmp = seed;
            seed_tmp[k].emplace_back(node_idx[i]);
            auto obj_tmp = query(seed_tmp);
            if (obj_tmp > max_obj)
            {
#pragma omp critical
                {
                    if (obj_tmp > max_obj)
                    {
                        max_obj = obj_tmp;
                        sel_k = k;
                    }
                }
            }
        }

        if (sel_k == Constants::K)
            continue;
        
        seed[sel_k].emplace_back(node_idx[i]);
    }

    auto current_obj = query(seed);

    spdlog::info("End DEGREE: obj {}", current_obj);

    return current_obj;
}