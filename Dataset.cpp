#include "Dataset.h"
#include <iostream>
#include <fstream>
#include <algorithm> // std::sort, std::stable_sort
#include <queue>
#include <stdio.h>
#include <sstream>
#include <numeric>

#include "Constants.h"

Dataset::Dataset()
{
}

void Dataset::read_network(const int &num_nodes, const string &file_name)
{
	clear();
	this->num_nodes = num_nodes;
	neighbors = vector<vector<uint>>(num_nodes);
	neighbor_edge_weight = vector<vector<uint>>(num_nodes);

	// initiate preferences for influence maximization
	if (Constants::APPLICATION == APP_TYPE::Social)
	{
		preferences = vector<vector<uint>>(num_nodes, vector<uint>(Constants::K, 0));
		// vector<uint> ini(Constants::K);

		// for (auto i = 0; i < Constants::K; ++i)
		// {
		// 	ini[i] = i + 1;
		// }

#pragma omp parallel for
		for (auto i = 0; i < num_nodes; ++i)
		{
			// preferences[i] = ini;
			// // random shuffle
			// random_shuffle(preferences[i].begin(), preferences[i].end());
			for (auto k = 0; k < Constants::K; ++k)
			{
				preferences[i][k] =
					MpCommon::getInstance()->randomInThread(omp_get_thread_num()) % Constants::K + 1;
			}
		}
	}

	ifstream is(file_name);
	is.seekg(0, is.end);
	long bufSize = is.tellg();
	is.seekg(0, is.beg);
	int item = 0;

	char *buffer = new char[bufSize];

	is.read(buffer, bufSize);
	is.close();

	std::string::size_type sz = 0;
	long sp = 0;
	uint start_id, end_id;
	bool is_start = true;
	uint id = 0;
	uint s_id, e_id; // used to stored ordered id of startId and endId
	uint edge_id = 0;

	while (sp < bufSize)
	{
		char c = buffer[sp];
		item = item * 10 + c - 48;
		sp++;
		if (sp == bufSize || (sp < bufSize && (buffer[sp] < 48 || buffer[sp] > 57)))
		{
			while (sp < bufSize && (buffer[sp] < 48 || buffer[sp] > 57))
				sp++;

			if (is_start)
			{
				start_id = item;
				is_start = false;
			}
			else
			{
				end_id = item;
				is_start = true;

				if (start_id != end_id)
				{
					auto const s_id_it = map_node_id.find(start_id);
					if (s_id_it == map_node_id.end())
					{
						map_node_id[start_id] = id;
						s_id = id;
						id++;
						if (id % 10000 == 0)
							spdlog::info("Read {} nodes", id);
					}
					else
					{
						s_id = s_id_it->second;
					}

					auto const e_id_it = map_node_id.find(end_id);
					if (e_id_it == map_node_id.end())
					{
						map_node_id[end_id] = id;
						e_id = id;
						id++;
						if (id % 10000 == 0)
							spdlog::info("Read {} nodes", id);
					}
					else
					{
						e_id = e_id_it->second;
					}

					// undirected graph
					neighbors[s_id].emplace_back(e_id);
					neighbors[e_id].emplace_back(s_id);

					// auto generate edge weight
					auto r = rand() % 1000;
					neighbor_edge_weight[s_id].emplace_back(r);
					neighbor_edge_weight[e_id].emplace_back(r);
				}
			}
			item = 0;
		}
	}

	spdlog::info("Finish reading graph of {} file, {} nodes", file_name, num_nodes);

	// generate sample graph for influence maximization
	if (Constants::APPLICATION == APP_TYPE::Social)
	{
		sample_graph_realization(Constants::NUM_SAMPLES);
		spdlog::info("Finish sampling {} graph realizations", Constants::NUM_SAMPLES);
	}

	delete[] buffer;
}

double Dataset::get_influence(const Seed &seed) const
{
	double re = 0.0;

#pragma omp parallel for
	for (int i = 0; i < sample_graphs.size(); ++i)
	{
		auto const &sample = sample_graphs[i];
		// counted if a node was influenced by at least 1 topic
		vector<bool> global_counted(num_nodes, false);
		uint inf_count = 0;

		for (auto k = 0; k < Constants::K; ++k)
		{
			auto const &sample_k = sample[k];
			auto const &seed_k = seed[k];
			// counted if a node was influenced in this topic
			vector<bool> local_counted(num_nodes, false);
			queue<uint> q;
			for (auto const &v : seed_k)
			{
				local_counted[v] = true;
				q.push(v);
				if (!global_counted[v])
				{
					global_counted[v] = true;
					++inf_count;
				}
			}
			while (!q.empty())
			{
				auto u = q.front();
				q.pop();
				auto const &nei = sample_k[u];
				for (auto const &v : nei)
				{
					if (local_counted[v])
						continue;
					if (!global_counted[v])
					{
						global_counted[v] = true;
						++inf_count;
					}
					q.push(v);
					local_counted[v] = true;
				}
			}
		}

#pragma omp critical
		{
			re += inf_count;
		}
	}

	return re / sample_graphs.size();
}

void Dataset::clear()
{
	map_node_id.clear();
	neighbors.clear();
	preferences.clear();
	sample_graphs.clear();
	neighbor_edge_weight.clear();
}

void Dataset::sample_graph_realization(const int &num_samples)
{
	auto common_ins = MpCommon::getInstance();

#pragma omp parallel for
	for (int i = 0; i < num_samples; ++i)
	{
		SampleKgraphs sample = vector<vector<set<uint>>>(Constants::K);
		for (auto k = 0; k < Constants::K; ++k)
		{
			vector<set<uint>> sample_nei = vector<set<uint>>(num_nodes);
			for (auto u = 0; u < num_nodes; ++u)
			{
				for (auto const &v : neighbors[u])
				{
					double r = MpCommon::getInstance()
								   ->randomInThread(omp_get_thread_num()) %
							   1000;
					r = r / 1000.0;
					if (r > ((double)preferences[v][k]) / (Constants::K * neighbors[v].size()))
						continue;
					sample_nei[u].emplace(v);
				}
			}
			sample[k] = sample_nei;
		}

#pragma omp critical
		{
			sample_graphs.emplace_back(sample);
		}
	}

	spdlog::info("Finish generating {} samples", num_samples);
}

uint Dataset::get_data_size() const
{
	return num_nodes;
}

uint Dataset::get_num_cross_edges(const Seed &seed) const
{
	uint re = 0;
	vector<uint> par(num_nodes, num_nodes);

#pragma omp parallel for
	for (auto k = 0; k < Constants::K; ++k)
	{
		for (auto const &e : seed[k])
		{
			par[e] = k;
		}
	}

	// 	vector<uint> count_cross(num_nodes, 0);

	// #pragma omp parallel for
	// 	for (auto e = 0; e < num_nodes; ++e)
	// 	{
	// 		if (par[e] == num_nodes)
	// 			continue;
	// 		for (auto const &v : neighbors[e])
	// 		{
	// 			if (par[v] != par[e])
	// 				++count_cross[e];
	// 		}
	// 	}

	vector<uint> count_cross(Constants::K, 0);
#pragma omp parallel for
	for (auto k = 0; k < Constants::K; ++k)
	{
		for (auto const &e : seed[k])
		{
			for (auto const &v : neighbors[e])
			{
				if (par[v] != k)
					++count_cross[k];
			}
		}
	}
	return std::accumulate(count_cross.begin(), count_cross.end(), 0);
}

uint Dataset::get_weight_cross_edges(const Seed &seed) const
{
	uint re = 0;
	vector<uint> par(num_nodes, num_nodes);

#pragma omp parallel for
	for (auto k = 0; k < Constants::K; ++k)
	{
		for (auto const &e : seed[k])
		{
			par[e] = k;
		}
	}

	vector<uint> weight_cross(Constants::K, 0);
#pragma omp parallel for
	for (auto k = 0; k < Constants::K; ++k)
	{
		for (auto const &e : seed[k])
		{
			for (auto i = 0; i < neighbors[e].size(); ++i)
			{
				auto const &v = neighbors[e][i];
				if (par[v] != k)
					weight_cross[k] += neighbor_edge_weight[e][i];
			}
		}
	}
	return std::accumulate(weight_cross.begin(), weight_cross.end(), 0);
}

vector<uint> Dataset::get_node_degrees() const
{
	vector<uint> re(num_nodes, 0);
	for (auto i = 0; i < num_nodes; ++i)
	{
		re[i] = neighbors[i].size();
	}
	return re;
}