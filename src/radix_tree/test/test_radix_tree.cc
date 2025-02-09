#include "catch.hpp"

#include "../radix_tree.h"

TEST_CASE( "Radix Tree Node", "[radix_tree_node]" ) {
  const k_size_t K = 16;
  
  auto block_tree = BlockRadixTreeNode<uint16_t, 4, K>(); 
  cout << block_tree << endl;
}

TEST_CASE( "Radix Tree", "[radix_tree]") {
  const k_size_t K = 16;

  auto tree = BlockRadixTree<uint16_t, K>();
  tree.InsertElement(bitset<K>(10));
  tree.InsertElement(bitset<K>(10));

  REQUIRE(tree.root.elems.nr_elems() == 1);

  cout << "RadixTree" << endl;
  cout << tree.root << endl;
  tree.InsertElement(bitset<K>(11)); // Insert 00001011
  tree.InsertElement(bitset<K>(110));// Insert 01101110
  tree.InsertElement(bitset<K>(210));// Insert 11010010
  tree.InsertElement(bitset<K>(15)); // Insert 00001111

  cout << tree.root << endl;
  tree.Compact(0);
  cout << tree.root << endl;
  REQUIRE(tree.root.elems.nr_elems() == 3);
}
