#ifndef BPTREE_HPP
#define BPTREE_HPP

#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>

using namespace std;

// Configuration
const int ORDER = 4; // Max children

template <typename KeyType, typename ValueType>
struct Node {
    bool is_leaf;
    vector<KeyType> keys;
    
    Node(bool leaf) : is_leaf(leaf) {}
    virtual ~Node() = default;
};

template <typename KeyType, typename ValueType>
struct InternalNode : public Node<KeyType, ValueType> {
    vector<Node<KeyType, ValueType>*> children;

    InternalNode() : Node<KeyType, ValueType>(false) {}
};

template <typename KeyType, typename ValueType>
struct LeafNode : public Node<KeyType, ValueType> {
    vector<vector<ValueType>> values; // Map key -> list of values (handling duplicates)
    LeafNode* next = nullptr;

    LeafNode() : Node<KeyType, ValueType>(true) {}
};

template <typename KeyType, typename ValueType>
class BPlusTree {
private:
    Node<KeyType, ValueType>* root;

    void splitLeaf(LeafNode<KeyType, ValueType>* leaf, InternalNode<KeyType, ValueType>* parent) {
        LeafNode<KeyType, ValueType>* newLeaf = new LeafNode<KeyType, ValueType>();
        int splitIndex = (ORDER + 1) / 2;

        for (int i = splitIndex; i < leaf->keys.size(); ++i) {
            newLeaf->keys.push_back(leaf->keys[i]);
            newLeaf->values.push_back(leaf->values[i]);
        }
        leaf->keys.resize(splitIndex);
        leaf->values.resize(splitIndex);

        newLeaf->next = leaf->next;
        leaf->next = newLeaf;

        KeyType splitKey = newLeaf->keys[0];
        insertIntoInternal(splitKey, parent, newLeaf);
    }

    void splitInternal(InternalNode<KeyType, ValueType>* node, InternalNode<KeyType, ValueType>* parent) {
        InternalNode<KeyType, ValueType>* newNode = new InternalNode<KeyType, ValueType>();
        int splitIndex = (ORDER + 1) / 2;

        KeyType upKey = node->keys[splitIndex];

        for (int i = splitIndex + 1; i < node->keys.size(); ++i) {
            newNode->keys.push_back(node->keys[i]);
        }
        for (int i = splitIndex + 1; i < node->children.size(); ++i) {
            newNode->children.push_back(node->children[i]);
        }

        node->keys.resize(splitIndex);
        node->children.resize(splitIndex + 1);

        insertIntoInternal(upKey, parent, newNode);
    }

    void insertIntoInternal(KeyType key, InternalNode<KeyType, ValueType>* parent, Node<KeyType, ValueType>* childRight) {
        if (parent == nullptr) {
            InternalNode<KeyType, ValueType>* newRoot = new InternalNode<KeyType, ValueType>();
            newRoot->keys.push_back(key);
            newRoot->children.push_back(root);
            newRoot->children.push_back(childRight);
            root = newRoot;
            return;
        }

        auto it = upper_bound(parent->keys.begin(), parent->keys.end(), key);
        int index = distance(parent->keys.begin(), it);

        parent->keys.insert(it, key);
        parent->children.insert(parent->children.begin() + index + 1, childRight);

        if (parent->keys.size() >= ORDER) {
            InternalNode<KeyType, ValueType>* grandParent = findParent(root, parent);
            splitInternal(parent, grandParent);
        }
    }

    InternalNode<KeyType, ValueType>* findParent(Node<KeyType, ValueType>* curr, Node<KeyType, ValueType>* child) {
        if (curr->is_leaf || curr == child) return nullptr;
        InternalNode<KeyType, ValueType>* internal = static_cast<InternalNode<KeyType, ValueType>*>(curr);

        for (size_t i = 0; i <= internal->keys.size(); ++i) {
            if (internal->children[i] == child) return internal;
            if (!internal->children[i]->is_leaf) {
                InternalNode<KeyType, ValueType>* potential = findParent(internal->children[i], child);
                if (potential) return potential;
            }
        }
        return nullptr;
    }

    LeafNode<KeyType, ValueType>* findLeaf(KeyType key) {
        if (!root) return nullptr;
        Node<KeyType, ValueType>* curr = root;
        while (!curr->is_leaf) {
            InternalNode<KeyType, ValueType>* internal = static_cast<InternalNode<KeyType, ValueType>*>(curr);
            auto it = upper_bound(internal->keys.begin(), internal->keys.end(), key);
            int idx = distance(internal->keys.begin(), it);
            curr = internal->children[idx];
        }
        return static_cast<LeafNode<KeyType, ValueType>*>(curr);
    }

public:
    BPlusTree() {
        root = new LeafNode<KeyType, ValueType>();
    }

    void insert(KeyType key, ValueType value) {
        LeafNode<KeyType, ValueType>* leaf = findLeaf(key);
        
        auto it = lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        int index = distance(leaf->keys.begin(), it);

        // Check if key exists
        if (it != leaf->keys.end() && *it == key) {
            leaf->values[index].push_back(value);
            return;
        }

        leaf->keys.insert(it, key);
        leaf->values.insert(leaf->values.begin() + index, {value});

        if (leaf->keys.size() >= ORDER) {
            InternalNode<KeyType, ValueType>* parent = findParent(root, leaf);
            splitLeaf(leaf, parent);
        }
    }

    vector<ValueType> search(KeyType key) {
        LeafNode<KeyType, ValueType>* leaf = findLeaf(key);
        for (size_t i = 0; i < leaf->keys.size(); ++i) {
            if (leaf->keys[i] == key) {
                return leaf->values[i];
            }
        }
        return {};
    }

    vector<ValueType> rangeSearch(KeyType start, KeyType end) {
        vector<ValueType> results;
        LeafNode<KeyType, ValueType>* leaf = findLeaf(start);
        
        while (leaf) {
            for (size_t i = 0; i < leaf->keys.size(); ++i) {
                if (leaf->keys[i] >= start && leaf->keys[i] <= end) {
                    results.insert(results.end(), leaf->values[i].begin(), leaf->values[i].end());
                }
                if (leaf->keys[i] > end) return results;
            }
            leaf = leaf->next;
        }
        return results;
    }

    void remove(KeyType key, ValueType value) {
        LeafNode<KeyType, ValueType>* leaf = findLeaf(key);
        for (size_t i = 0; i < leaf->keys.size(); ++i) {
            if (leaf->keys[i] == key) {
                auto& vals = leaf->values[i];
                vals.erase(std::remove(vals.begin(), vals.end(), value), vals.end());
                if (vals.empty()) {
                    leaf->keys.erase(leaf->keys.begin() + i);
                    leaf->values.erase(leaf->values.begin() + i);
                }
                return;
            }
        }
    }
    
    // For cleaning up during destruction, a recursive deletion would be needed. 
    // Omitting for brevity in this snippet as OS reclaims memory on exit for this scope.
};

#endif
