#include <iostream>
#include "../include/database.hpp"
#include "../include/ml.hpp"

using namespace std;

int main() {
    cout << "=== Antigravity DB ML Demo ===\n\n";

    Database db;

    // --- Regression Demo ---
    cout << "[1] Linear Regression Demo (Predicting y = 2x)\n";
    db.createTable("linear_data", {"x", "y"});
    Table* t1 = db.getTable("linear_data");
    
    // Train on: (1,2), (2,4), (3,6)
    t1->insert({{"x", "1"}, {"y", "2"}});
    t1->insert({{"x", "2"}, {"y", "4"}});
    t1->insert({{"x", "3"}, {"y", "6"}});
    
    // Predict for x=10 -> should be 20
    double prediction = ML::predictLinear(t1, "y", "x", 10.0);
    cout << "Predicting y for x=10: " << prediction << "\n\n";


    // --- Clustering Demo ---
    cout << "[2] K-Means Clustering Demo\n";
    db.createTable("points", {"x", "y"});
    Table* t2 = db.getTable("points");
    
    // Cluster A: (0,0), (1,1)
    t2->insert({{"x", "0"}, {"y", "0"}});
    t2->insert({{"x", "1"}, {"y", "1"}});
    
    // Cluster B: (10,10), (11,11)
    t2->insert({{"x", "10"}, {"y", "10"}});
    t2->insert({{"x", "11"}, {"y", "11"}});
    
    cout << "Running K-Means (K=2)...\n";
    map<int, vector<int>> clusters = ML::kMeans(t2, 2, {"x", "y"});
    
    for (auto const& [id, members] : clusters) {
        cout << "Cluster " << id << ": ";
        for (int m : members) cout << m << " ";
        cout << "\n";
    }

    return 0;
}
