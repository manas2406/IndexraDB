#include "../../include/nlq_ml/embed.hpp"
#include <cmath>
#include <sstream>
#include <map>
#include <numeric>

using namespace std;

namespace NLQ_ML {

    // Simple hash-based vectorizer (Hashing Trick)
    // Dimension 100
    static const int DIM = 100;

    vector<double> EmbedModel::embed(const string& text) {
        vector<double> vec(DIM, 0.0);
        stringstream ss(text);
        string word;
        while(ss >> word) {
            size_t hash = std::hash<string>{}(word);
            vec[hash % DIM] += 1.0;
        }
        return vec;
    }

    double EmbedModel::similarity(const string& t1, const string& t2) {
        vector<double> v1 = embed(t1);
        vector<double> v2 = embed(t2);

        double dot = 0.0, mag1 = 0.0, mag2 = 0.0;
        for (int i = 0; i < DIM; ++i) {
            dot += v1[i] * v2[i];
            mag1 += v1[i] * v1[i];
            mag2 += v2[i] * v2[i];
        }
        
        if (mag1 == 0 || mag2 == 0) return 0.0;
        return dot / (sqrt(mag1) * sqrt(mag2));
    }
}
