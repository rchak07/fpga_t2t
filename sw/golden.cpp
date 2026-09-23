#include <cassert>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "book.h"


struct Node {
    int feature_index, threshold, left_child, right_child, is_leaf, leaf_class;
};

std::vector<Node> load_tree(const std::string& path) {
    std::ifstream in(path);
    int n;
    in >> n;
    std::vector<Node> tree(n);
    for (auto& nd : tree) {
        in >> nd.feature_index >> nd.threshold >> nd.left_child
           >> nd.right_child >> nd.is_leaf >> nd.leaf_class;
    }
    return tree;
}

int predict(const std::vector<Node>& tree, int f[4]) {
    int idx = 0;
    while (!tree[idx].is_leaf) {
        int feat = tree[idx].feature_index;
        idx = (f[feat] < tree[idx].threshold) ? tree[idx].left_child : tree[idx].right_child;
    }
    return tree[idx].leaf_class;
}

int main() {
    // 1. Read every line of data/test.hex into a vector<uint8_t>.
    std::ifstream in("data/test.hex");
    std::vector<uint8_t> bytes;
    std::string line;
    while(std::getline(in, line)){
        uint8_t b = std::stoi(line, nullptr, 16);
        bytes.push_back(b);
    }
    assert(bytes.size()%8 == 0);
    // 2. Load the tree from data/tree.txt.
    std::vector<Node> tree = load_tree("data/tree.txt");
    // 3. Open the three output files.
    std::ofstream exp_updates("data/expected_updates.txt");
    std::ofstream exp_features("data/expected_features.txt");
    std::ofstream exp_orders("data/expected_orders.txt");
    // 4. Loop over the byte vector 8 bytes at a time (one message per iteration):
    //    - assert bytes[0] == 0x51
    //    - side = bytes[1]
    //    - price = (bytes[2]<<24)|(bytes[3]<<16)|(bytes[4]<<8)|bytes[5]
    //    - qty   = (bytes[6]<<8)|bytes[7]
    //    - write "idx side price qty" to expected_updates.txt
    //    - Features feat = book.update(side, price, qty)
    //    - int f[4] = {feat.f0, feat.f1, feat.f2, feat.f3};
    //    - int pred = predict(tree, f);
    //    - write "idx f0 f1 f2 f3 pred" to expected_features.txt
    //    - if (pred == 1) write "idx <current ask> 100" to expected_orders.txt
    Book book;
    for(size_t i = 0; i < bytes.size(); i+=8){
        assert(bytes[i] == 0x51);
        int side = bytes[i+1];
        int price = (bytes[i+2]<<24) | (bytes[i+3]<<16) | (bytes[i+4]<<8) | (bytes[i+5]);
        int qty = (bytes[i+6]<<8) | bytes[i+7];
        int idx = i/8;
        exp_updates << idx << " "
                    << side << " "
                    << price << " "
                    << qty << "\n";
        Features feat = book.update(side, price, qty);
        int f[4] = {feat.f0, feat.f1, feat.f2, feat.f3};
        int pred = predict(tree, f);
        exp_features << idx << " "
                     << f[0] << " "
                     << f[1] << " "
                     << f[2] << " "
                     << f[3] << " "
                     << pred << "\n";
        if(pred == 1){
            exp_orders << idx << " "
                       << book.ask << " "
                       << 100 << "\n"; 
        } 

    }
}