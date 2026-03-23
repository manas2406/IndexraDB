#ifndef ML_HPP
#define ML_HPP

#include "table.hpp"
#include <vector>
#include <string>
#include <map>

using namespace std;

// ML Utilities
namespace ML {

    // --- Linear Regression ---
    // Returns predicted target value for a given feature value.
    // Calculates slope/intercept on-the-fly.
    // Throws invalid_argument if columns not found or non-numeric.
    double predictLinear(Table* t, string targetCol, string featureCol, double featureVal);

    // --- K-Means Clustering ---
    // Returns a map: ClusterID -> Vector of RowIndices belonging to that cluster.
    // k: number of clusters
    // cols: numeric columns to use for clustering dimensions
    // iterations: max iterations (default 10)
    map<int, vector<int>> kMeans(Table* t, int k, vector<string> cols, int iterations = 10);

}

#endif
