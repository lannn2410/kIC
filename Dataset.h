#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include "MpCommon.h"

using namespace std;

// use for sampling graph realization in social network
// k -> node id -> list of neighbors
typedef vector<vector<set<uint>>> SampleKgraphs;

// k -> node ids
typedef vector<vector<uint>> Seed;

class Dataset
{
public:
	Dataset();

	uint get_data_size() const;

	void read_network(const int &num_nodes, const string &file_name);
	vector<uint> get_node_degrees() const;

	// for social network
	double get_influence(const Seed &seed /* k-> node ids*/) const;

	// for max (weight) cut
	uint get_num_cross_edges(const Seed &seed) const;
	uint get_weight_cross_edges(const Seed &seed) const;
private:
	void clear();
	// for social network application
	void sample_graph_realization(const int &num_samples);
	uint num_nodes;
	map<uint, uint> map_node_id;			  // map from true id -> ordered id (used for read graph from file)
	vector<vector<uint>> neighbors;			  // node_id -> list of neighbors
	
	// map from node_id -> preferences on each topic k
	// this impacts the weight of an out-edge with adopting different product.
	// If an user's preference to a topic i is j, 
	// then it has prob j / (k * d) to be impacted by his neighbor in topic i
	// where d is the user's degree 
	vector<vector<uint>> preferences;

	// map from node_id -> neighbor idx -> edge weight
	// use for max weight cut
	vector<vector<uint>> neighbor_edge_weight; 
	
	// pre-sample graph realization, for fast computation
	vector<SampleKgraphs> sample_graphs; 
};
