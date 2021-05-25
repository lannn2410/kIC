#include "SimpleStream.h"

SimpleStream::SimpleStream(const Dataset &d) : Algorithm(d)
{
}

SimpleStream::~SimpleStream()
{
}

double SimpleStream::get_solutions(Seed &seed)
{
    auto const num_nodes = data.get_data_size();
    seed.clear();
    seed = Seed(Constants::K);

    double current_obj = query(seed);

    spdlog::info("Start SIMPLE STREAM: current obj {}", current_obj);

    double const t = ((double)(Constants::B * Constants::K)) / num_nodes;
    set<uint> I;
    for (auto i = 0; i < Constants::K; ++i)
    {
        I.emplace(i);
    }
    for (auto i = 0; i < num_nodes; ++i)
    {
        double r = rand() % 1000;
        r /= 1000;
        if (r > t)
            continue;
        auto const k_idx = rand() % I.size();
        auto const k = common_ins->get_element_by_index_from_set(k_idx, I);
        seed[k].emplace_back(i);

        if (seed[k].size() < Constants::B)
            continue;
        I.erase(k);
        if (I.empty())
            break;
    }

    current_obj = query(seed);

    spdlog::info("End SIMPLE STREAM: obj {}", current_obj);

    return current_obj;
}