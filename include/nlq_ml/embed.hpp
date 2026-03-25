#ifndef NLQ_EMBED_MODEL_HPP
#define NLQ_EMBED_MODEL_HPP

#include <string>
#include <vector>

namespace NLQ_ML {
    class EmbedModel {
    public:
        // Returns a vector representation of text (BoW)
        std::vector<double> embed(const std::string& text);
        
        // Returns cosine similarity between two texts
        double similarity(const std::string& t1, const std::string& t2);
    };
}

#endif
