#include "Algorithm.h"

Algorithm::Algorithm(const Dataset &d)
    : data(d), num_queries(0)
{
    common_ins = MpCommon::getInstance();
}

uint Algorithm::get_num_queries() const
{
    return num_queries;
}

double Algorithm::query(const Seed &s)
{
#pragma omp critical
    {
        ++num_queries;
    }

    if (Constants::APPLICATION == APP_TYPE::Social)
        return data.get_influence(s);
    // return data.get_num_cross_edges(s);
    return data.get_weight_cross_edges(s);
}

void Algorithm::reset_num_queries()
{
    num_queries = 0;
}

bool Algorithm::is_obj_monotone() const
{
    return Constants::APPLICATION == APP_TYPE::Social;
}

void Algorithm::non_monotone_submodular_alg(Seed &seed, uint const &k,
                                            vector<bool> &in_seed, double &current_obj)
{
    auto const num_nodes = data.get_data_size();
    auto const budget = Constants::B - seed[k].size();
    spdlog::info("Start non monotone submodular selection, b {} k {} current obj {}",
                 budget, k, current_obj);
    for (auto i = 0; i < budget; ++i)
    {
        vector<uint> e_idx(num_nodes);
        vector<double> gains(num_nodes, 0.0);
        vector<double> new_obj(num_nodes, 0.0);
        for (auto e = 0; e < num_nodes; ++e)
        {
            e_idx[e] = e;
            if (in_seed[e])
                continue;
            auto seed_tmp = seed;
            seed_tmp[k].emplace_back(e);
            new_obj[e] = query(seed_tmp);
            gains[e] = new_obj[e] - current_obj;
        }

        // sort elements in descending order w.r.t gains
        sort(e_idx.begin(), e_idx.end(), [&gains](size_t i1, size_t i2) {
            return gains[i1] > gains[i2];
        });

        // get elements of maximum gains, but only get the one with gain > 0
        vector<uint> lottery(budget, num_nodes);
        for (auto j = 0; j < budget; ++j)
        {
            if (gains[e_idx[j]] < 0)
                break;
            lottery[j] = e_idx[j];
        }
        auto r = common_ins->randomInThread(omp_get_thread_num()) % budget;
        if (lottery[r] == num_nodes)
        {
            spdlog::info("Dummy is picked in lottery");
            continue;
        }
        seed[k].emplace_back(lottery[r]);
        in_seed[lottery[r]] = true;
        current_obj = new_obj[lottery[r]];
        spdlog::info("e {} is picked to put to k {}, current obj {}",
                     lottery[r], k, current_obj);
    }
    spdlog::info("End non monotone submodular selection, current obj {}", current_obj);
}

tuple<double, uint> Algorithm::refine_seed(const Seed &seed)
{
    double new_obj = query(seed);
    auto num_nodes = data.get_data_size();
    uint violate_count = 0;

    auto refine_seed = seed;
    set<uint> I;
    for (auto i = 0; i < Constants::K; ++i)
    {
        if (refine_seed[i].size() > Constants::B)
        {
            I.emplace(i);
            violate_count += (refine_seed[i].size() - Constants::B);
        }
    }

    spdlog::info("Start refining solution");

    while (!I.empty())
    {
        double max_obj = 0.0;
        uint sel_idx = num_nodes, sel_k = Constants::K;
        for (auto const &k : I)
        {
#pragma omp parallel for
            for (auto i = 0; i < refine_seed[k].size(); ++i)
            {
                auto seed_tmp = refine_seed;
                seed_tmp[k].erase(seed_tmp[k].begin() + i);
                auto obj_tmp = query(seed_tmp);
                if (obj_tmp > max_obj)
                {
#pragma omp critical
                    {
                        if (obj_tmp > max_obj)
                        {
                            max_obj = obj_tmp;
                            sel_idx = i;
                            sel_k = k;
                        }
                    }
                }
            }
        }

        if (sel_idx == num_nodes || sel_k == Constants::K)
        {
            throw "Something wrong";
        }

        new_obj = max_obj;

        spdlog::info("Remove e {} out of k {} new obj {}", refine_seed[sel_k][sel_idx], sel_k, new_obj);

        refine_seed[sel_k].erase(refine_seed[sel_k].begin() + sel_idx);

        if (refine_seed[sel_k].size() <= Constants::B)
        {
            I.erase(sel_k);
        }
    }

    spdlog::info("End refining, new obj {} violate count {}", new_obj, violate_count);

    return make_tuple(new_obj, violate_count);
}