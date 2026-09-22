// train_tree.cpp
//
// Trains a small ID3 decision tree on a toy 4-feature / binary-class
// dataset, flattens it into an array representation, and exports that
// array as a SystemVerilog header (tree_data.svh) for the hardware
// tree-walker to consume.
//
// Node format (mirrors the SV side):
//   feature_index : which of the 4 features this node checks (0-3)
//   threshold     : compare feature value against this
//   left_child    : index of left child   (taken if feature <  threshold)
//   right_child   : index of right child  (taken if feature >= threshold)
//   is_leaf       : true if this node is a leaf
//   leaf_class    : valid only if is_leaf

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <vector>

// ---------- Data ----------

struct Sample {
    int features[4]; 
    int label;     
};

std::vector<Sample> load_csv(const std::string& path) {
    std::vector<Sample> rows;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        Sample r;
        sscanf(line.c_str(), "%d,%d,%d,%d,%d",
               &r.features[0], &r.features[1], &r.features[2], &r.features[3], &r.label);
        rows.push_back(r);
    }
    return rows;
}

auto train_rows = load_csv("data/train.csv");
auto test_rows = load_csv("data/test.csv");


// ---------- Entropy ----------

double entropy(const std::vector<Sample>& s) {
    if (s.empty()) {
        return 0.0;
    }
    int total = s.size();
    double p0 = 0;
    double p1 = 0;
    for (int i = 0; i < total; ++i) {
        if (s[i].label == 1) {
            p1++;
        } else {
            p0++;
        }
    }
    p1 = p1 / total;
    p0 = p0 / total;
    double ent = 0.0;
    if (p1 > 0) {
        ent += -p1 * log2(p1);
    }
    if (p0 > 0) {
        ent += -p0 * log2(p0);
    }
    return ent;
}

// ---------- Best split ----------

struct SplitResult {
    bool found = false;
    uint8_t feature_index = 0;
    uint8_t threshold = 0;
    double gain = -1.0;
};

SplitResult best_split(const std::vector<Sample>& s) {
    SplitResult best;
    double bef_entropy = entropy(s);
    for (int i = 0; i < 4; ++i) {
        std::set<uint8_t> dist_vals;
        std::vector<uint8_t> values;
        std::vector<uint8_t> cand_thresh;
        std::vector<Sample> left;
        std::vector<Sample> right;

        for (size_t j = 0; j < s.size(); ++j) {
            dist_vals.insert((s[j].features)[i]);
        }
        for (auto val : dist_vals) {
            values.push_back(val);
        }
        for (size_t j = 0; j < values.size(); ++j) {
            if (j != values.size() - 1) {
                uint8_t thresh = (values[j] + values[j + 1]) / 2;
                cand_thresh.push_back(thresh);
            }
        }
        for (size_t j = 0; j < cand_thresh.size(); j++) {
            left.clear();
            right.clear();
            for (size_t k = 0; k < s.size(); ++k) {
                if ((s[k].features)[i] < cand_thresh[j]) {
                    left.push_back(s[k]);
                } else {
                    right.push_back(s[k]);
                }
            }
            if (!left.empty() && !right.empty()) {
                double left_entropy = entropy(left);
                double right_entropy = entropy(right);
                double left_frac = (double)left.size() / s.size();
                double right_frac = (double)right.size() / s.size();
                double gain = bef_entropy - (left_frac * left_entropy + right_frac * right_entropy);
                if (gain > best.gain) {
                    best.found = true;
                    best.feature_index = i;
                    best.threshold = cand_thresh[j];
                    best.gain = gain;
                }
            }
        }
    }
    return best;
}

// ---------- Stopping-condition helpers ----------

bool is_pure(const std::vector<Sample>& s) {
    uint8_t label = s[0].label;
    for (size_t i = 1; i < s.size(); ++i) {
        if (s[i].label != label) {
            return false;
        }
    }
    return true;
}

uint8_t majority_class(const std::vector<Sample>& s) {
    int label0 = 0, label1 = 0;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i].label == 0) {
            label0++;
        } else {
            label1++;
        }
    }
    if (label0 >= label1) {
        return 0;
    } else {
        return 1;
    }
}

// ---------- Recursive tree ----------

struct TreeNode {
    bool is_leaf = false;
    uint8_t leaf_class = 0;
    uint8_t feature_index = 0;
    uint8_t threshold = 0;
    std::unique_ptr<TreeNode> left;
    std::unique_ptr<TreeNode> right;
};

std::unique_ptr<TreeNode> build_tree(const std::vector<Sample>& s, int depth, int max_depth) {
    auto node = std::make_unique<TreeNode>();
    if (is_pure(s) || depth >= max_depth || s.size() <= 1) {
        node->is_leaf = true;
        node->leaf_class = majority_class(s);
        int c1 = 0;
        for (auto& r : s) c1 += r.label;
        std::cerr << "LEAF depth=" << depth << " n=" << s.size()
                   << " class1=" << c1 << " class0=" << (s.size() - c1)
                   << " -> assigned " << (int)node->leaf_class << "\n";
        return node;
    }

    SplitResult split = best_split(s);

    if (!split.found || split.gain <= 0.0) {
        node->is_leaf = true;
        node->leaf_class = majority_class(s);
        return node;
    }

    node->feature_index = split.feature_index;
    node->threshold = split.threshold;

    std::vector<Sample> left, right;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i].features[node->feature_index] < node->threshold) {
            left.push_back(s[i]);
        } else {
            right.push_back(s[i]);
        }
    }

    node->left = build_tree(left, depth + 1, max_depth);
    node->right = build_tree(right, depth + 1, max_depth);

    return node;
}

// ---------- Flattening ----------

struct FlatNode {
    uint8_t feature_index;
    uint8_t threshold;
    uint8_t left_child;
    uint8_t right_child;
    bool is_leaf;
    uint8_t leaf_class;
};

int flatten(TreeNode* node, std::vector<FlatNode>& out) {
    int my_index = out.size();
    out.push_back(FlatNode{});
    if (node->is_leaf) {
        out[my_index].is_leaf = true;
        out[my_index].leaf_class = node->leaf_class;
        return my_index;
    } else {
        out[my_index].left_child = static_cast<uint8_t>(flatten(node->left.get(), out));
        out[my_index].right_child = static_cast<uint8_t>(flatten(node->right.get(), out));
        out[my_index].feature_index = node->feature_index;
        out[my_index].threshold = node->threshold;
        return my_index;
    }
}

void dump_tree_txt(const std::vector<FlatNode>& tree, const std::string& path) {
    std::ofstream out(path);
    out << tree.size() << "\n";
    for (auto& f : tree) {
        out << (int)f.feature_index << " " << (int)f.threshold << " "
            << (int)f.left_child << " " << (int)f.right_child << " "
            << (f.is_leaf ? 1 : 0) << " " << (int)f.leaf_class << "\n";
    }
}

// ---------- Prediction on the flattened tree ----------

uint8_t predict(const std::vector<FlatNode>& tree, const int features[4]) {
    int idx = 0;
    while (!tree[idx].is_leaf) {
        int feature = tree[idx].feature_index;
        if (features[feature] < tree[idx].threshold) {
            idx = tree[idx].left_child;
        } else {
            idx = tree[idx].right_child;
        }
    }
    return tree[idx].leaf_class;
}

// ---------- SV export ----------

// NOTE: Icarus Verilog (v12.0 "devel"/"stable" builds, as of 2026) does not
// support the '{...} array-literal initializer on localparam/parameter
// unpacked arrays ("localparam must have a value" / "Invalid module item").
// So instead of:
//     localparam logic [7:0] FEATURE_IDX [0:N-1] = '{1,2,3};
// we declare a plain `logic` array and assign each element inside an
// `initial` block. Functionally equivalent (fixed data, set once, never
// changes during simulation) and works everywhere.
void export_sv_header(const std::vector<FlatNode>& tree, const std::string& path) {
    std::ofstream out(path);
    int n = static_cast<int>(tree.size());

    out << "// tree_data.svh -- auto-generated by train_tree.cpp\n";
    out << "// Flattened decision tree. Root is node 0.\n";
    out << "// Semantics: if (feature[feature_index] < threshold) go to left_child; else right_child.\n";
    out << "// NOTE: uses logic arrays + initial blocks (not localparam '{...}) for\n";
    out << "// compatibility with Icarus Verilog, which does not support array-literal\n";
    out << "// initializers on localparam/parameter unpacked arrays.\n\n";
    out << "localparam int NUM_NODES = " << n << ";\n\n";

    auto emit_decl = [&](const char* name, const char* type) {
        out << type << " " << name << " [0:NUM_NODES-1];\n";
    };
    emit_decl("FEATURE_IDX", "logic [7:0]");
    emit_decl("THRESHOLD",   "logic [7:0]");
    emit_decl("LEFT_CHILD",  "logic [7:0]");
    emit_decl("RIGHT_CHILD", "logic [7:0]");
    emit_decl("IS_LEAF",     "logic");
    emit_decl("LEAF_CLASS",  "logic [7:0]");
    out << "\n";

    out << "initial begin\n";
    for (int i = 0; i < n; i++) {
        const FlatNode& f = tree[i];
        out << "    FEATURE_IDX[" << i << "] = " << (int)f.feature_index << ";\n";
        out << "    THRESHOLD["   << i << "] = " << (int)f.threshold     << ";\n";
        out << "    LEFT_CHILD["  << i << "] = " << (int)f.left_child    << ";\n";
        out << "    RIGHT_CHILD[" << i << "] = " << (int)f.right_child   << ";\n";
        out << "    IS_LEAF["     << i << "] = " << (f.is_leaf ? "1'b1" : "1'b0") << ";\n";
        out << "    LEAF_CLASS["  << i << "] = " << (int)f.leaf_class    << ";\n";
    }
    out << "end\n";

    out.close();
}

// Also export the raw test vectors + expected predictions, so the SV
// testbench can cross-check against this exact C++ model.
void export_test_vectors(const std::vector<Sample>& data, const std::vector<FlatNode>& tree, const std::string& path) {
    std::ofstream out(path);
    out << "// test_vectors.svh -- auto-generated by train_tree.cpp\n";
    out << "// Each row: {f0, f1, f2, f3, expected_class}\n";
    out << "// NOTE: uses a logic array + initial block (not localparam '{...}) for\n";
    out << "// Icarus Verilog compatibility -- see tree_data.svh for details.\n\n";
    out << "localparam int NUM_VECTORS = " << data.size() << ";\n\n";
    out << "logic [7:0] TEST_VECTORS [0:NUM_VECTORS-1][0:4];\n\n";
    out << "initial begin\n";
    for (size_t i = 0; i < data.size(); i++) {
        uint8_t pred = predict(tree, data[i].features);
        out << "    TEST_VECTORS[" << i << "][0] = " << (int)data[i].features[0] << ";\n";
        out << "    TEST_VECTORS[" << i << "][1] = " << (int)data[i].features[1] << ";\n";
        out << "    TEST_VECTORS[" << i << "][2] = " << (int)data[i].features[2] << ";\n";
        out << "    TEST_VECTORS[" << i << "][3] = " << (int)data[i].features[3] << ";\n";
        out << "    TEST_VECTORS[" << i << "][4] = " << (int)pred << ";\n";
    }
    out << "end\n";
    out.close();
}

// ---------- Debug print ----------

void print_flat_tree(const std::vector<FlatNode>& tree) {
    std::cout << "Flattened tree (" << tree.size() << " nodes):\n";
    for (size_t i = 0; i < tree.size(); i++) {
        const FlatNode& n = tree[i];
        if (n.is_leaf) {
            std::cout << "  [" << i << "] LEAF class=" << (int)n.leaf_class << "\n";
        } else {
            std::cout << "  [" << i << "] feature[" << (int)n.feature_index << "] < "
                      << (int)n.threshold << " ? -> " << (int)n.left_child
                      << " : -> " << (int)n.right_child << "\n";
        }
    }
}

int main() {
    auto data = train_rows;
    auto root = build_tree(data, 0, /*max_depth=*/4);

    long long hi_total = 0, hi_up = 0, lo_total = 0, lo_up = 0;
    for (auto& s : train_rows) {
        if (s.features[1] >= 128) { hi_total++; hi_up += s.label; }
        else { lo_total++; lo_up += s.label; }
    }
    std::cout << "f1>=128: " << hi_up << "/" << hi_total << " = "
            << (100.0 * hi_up / hi_total) << "% up\n";
    std::cout << "f1<128:  " << lo_up << "/" << lo_total << " = "
            << (100.0 * lo_up / lo_total) << "% up\n";

    std::vector<FlatNode> flat;
    flatten(root.get(), flat);

    int pred1 = 0;
    for(auto& s : train_rows) if(predict(flat, s.features) == 1) pred1++;
    std::cout << "Tree predicts class 1 for " << pred1 << "/" << train_rows.size() << " rows\n";

    print_flat_tree(flat);

    std::cout << "\nTrain-set accuracy\n";
    int train_correct = 0;
    for (auto& s : train_rows) {
        uint8_t pred = predict(flat, s.features);
        train_correct += (pred == s.label);
    }
    std::cout << "Train: " << train_correct << "/" << train_rows.size() << " ("
            << (100.0 * train_correct / train_rows.size()) << "%)\n";

    std::cout << "\nTest-set accuracy:\n";
    int test_correct = 0;
    for (auto& s : test_rows) {
        uint8_t pred = predict(flat, s.features);
        test_correct += (pred == s.label);
    }
    std::cout << test_correct << "/" << test_rows.size() << " correct ("
            << (100.0 * test_correct / test_rows.size()) << "%)\n";

    dump_tree_txt(flat, "data/tree.txt");

    export_sv_header(flat, "tree_data.svh");
    export_test_vectors(data, flat, "test_vectors.svh");
    std::cout << "\nWrote tree_data.svh and test_vectors.svh\n";

    return 0;
}
