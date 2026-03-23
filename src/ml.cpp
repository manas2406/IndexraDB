#include "../include/ml.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>

using namespace std;

namespace ML {

    double predictLinear(Table* t, string targetCol, string featureCol, double featureVal) {
        if (!t) return 0.0;

        vector<double> X, Y;
        
        // Extract Data
        for (const auto& row : t->rows) {
            if (row.count("__deleted")) continue;
            try {
                if (row.count(targetCol) && row.count(featureCol)) {
                    double y = stod(row.at(targetCol));
                    double x = stod(row.at(featureCol));
                    X.push_back(x);
                    Y.push_back(y);
                }
            } catch (...) {
                // Ignore non-numeric rows (dirty data)
            }
        }

        if (X.empty()) {
            cout << "Error: No valid numeric data found for regression.\n";
            return 0.0;
        }

        size_t n = X.size();
        double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;

        for (size_t i = 0; i < n; ++i) {
            sumX += X[i];
            sumY += Y[i];
            sumXY += X[i] * Y[i];
            sumX2 += X[i] * X[i];
        }

        double denominator = (n * sumX2 - sumX * sumX);
        if (denominator == 0) return 0.0; // Vertical line

        double slope = (n * sumXY - sumX * sumY) / denominator;
        double intercept = (sumY - slope * sumX) / n;

        cout << "[Model] y = " << slope << "x + " << intercept << endl;
        
        return slope * featureVal + intercept;
    }

    // Helper for K-Means
    struct Point {
        int rowIdx;
        vector<double> features;
    };

    double distance(const vector<double>& p1, const vector<double>& p2) {
        double d = 0;
        for (size_t i = 0; i < p1.size(); ++i) {
            d += pow(p1[i] - p2[i], 2);
        }
        return sqrt(d);
    }

    map<int, vector<int>> kMeans(Table* t, int k, vector<string> cols, int iterations) {
        map<int, vector<int>> clusters;
        if (!t || k <= 0) return clusters;

        vector<Point> data;

        // 1. Load Data
        for (int i = 0; i < t->rows.size(); ++i) {
            if (t->rows[i].count("__deleted")) continue;
            
            Point p;
            p.rowIdx = i;
            bool valid = true;
            for (const string& col : cols) {
                try {
                    p.features.push_back(stod(t->rows[i].at(col)));
                } catch (...) {
                    valid = false; break;
                }
            }
            if (valid && !p.features.empty()) {
                data.push_back(p);
            }
        }

        if (data.size() < k) {
             cout << "Error: Not enough data points for " << k << " clusters.\n";
             return clusters;
        }

        // 2. Initialize Centroids (Pick first K points)
        vector<vector<double>> centroids;
        for (int i = 0; i < k; ++i) {
            centroids.push_back(data[i].features);
        }

        // 3. Loop
        for (int iter = 0; iter < iterations; ++iter) {
            // Clear clusters
            clusters.clear();
            for(int i=0; i<k; ++i) clusters[i] = {};

            // Assign points
            for (const auto& p : data) {
                double minDist = numeric_limits<double>::max();
                int clusterIdx = 0;

                for (int i = 0; i < k; ++i) {
                    double d = distance(p.features, centroids[i]);
                    if (d < minDist) {
                        minDist = d;
                        clusterIdx = i;
                    }
                }
                clusters[clusterIdx].push_back(p.rowIdx);
            }

            // Update Centroids
            for (int i = 0; i < k; ++i) {
                if (clusters[i].empty()) continue;
                
                vector<double> newCentroid(cols.size(), 0.0);
                for (int idx : clusters[i]) {
                    // Find point in data (inefficient O(N), but safe)
                    // We need the features of row idx.
                    // We can re-parse or store better. Since 'data' stores rowIdx, let's find it.
                    // But 'data' is vector. O(1) loop...
                    // Better: We iterate data above. 
                    // Let's iterate 'clusters' and look up in Table? Or store Point* in clusters?
                    // Re-parsing from table is slow.
                    // Let's just do a quick lookup optimization or accept inefficiency for this Proof of Concept.
                    // Actually, let's just loop locally.
                    
                    // Optim: store iterator or index in data? 
                    // Just re-fetch from Table to correspond to rowIdx
                     for (size_t d = 0; d < cols.size(); ++d) {
                         newCentroid[d] += stod(t->rows[idx].at(cols[d]));
                     }
                }
                
                for (size_t d = 0; d < cols.size(); ++d) {
                    centroids[i][d] = newCentroid[d] / clusters[i].size();
                }
            }
        }

        return clusters;
    }

}
