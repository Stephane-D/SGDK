
/* ------------------------------------------------------------ *
 * Debugging Modules Utilities Core								*
 * Huffman encoder implementation								*
 * (c) 2017-2018, 2024, Vladikcomper							*
 * ------------------------------------------------------------	*/

#pragma once

#include "IO.hpp"

#include <cassert>
#include <cstdint>
#include <functional>
#include <iterator>
#include <set>
#include <map>
#include <utility>


struct Huffman {

	/* ----------------------- *
	 * Structures declarations *
	 * ----------------------- */

	/* Simple node structure that is used to build a tree */
	struct Node {
		static inline int uid = 0;

		// Disable copy constructors to avoid duplicate IDs
		Node(const Node& node) noexcept = delete;
		Node& operator=(const Node& node) noexcept = delete;

		// Constructor: Unlinked Huffman tree node with the data
		Node(const uint16_t _data) noexcept
		: data(_data) {
			id = uid++;
			leaf[0] = leaf[1] = nullptr;	// specify leaves as unlinked (orphans)
		};

		// Constructor: Merge two specified nodes into a branch (a root for the passed nodes is constructed)
		Node(Node* A, Node* B) noexcept
		: data(0) {
			id = uid++;
			leaf[0] = A;	// link node A to the left leaf
			leaf[1] = B;	// link node B to the right leaf
		};

		// Destructor
		~Node() {
			delete leaf[0];		// destroy node structure in leaf 1
			delete leaf[1];		// destroy node structure in leaf 2
		};

		// Data structure
		uint16_t data;		// character code (or other value) that this node stores
		Node* leaf[2];		// connects the node to the underlying nodes, forming a binary tree
		int id;				// unique node identifier (mostly used for determinstic tree sorting)
	};

	/* Complete record of Huffman-encoded symbol */
	struct Record {
		Record( uint32_t _code, uint8_t _codeLength, uint16_t _data )
			: code(_code), codeLength(_codeLength), data(_data) {}

		// Data structure
		uint32_t code;
		uint8_t codeLength;
		uint16_t data;
	};

	/* Define type for organized Huffman records */
	struct sortByCode {
		bool operator () (const Record& A, const Record& B) const {
			if (A.code != B.code) {
				return A.code < B.code;
			}
			return A.codeLength < B.codeLength;
		}
	};
	typedef std::multiset<Record, sortByCode> RecordSet;


	/* ------------------------ *
	 * Functions implementation *
	 * ------------------------ */

	/**
	 * Recursive subroutine to render Huffman-codes based on pre-generated tree
	 */
	static void buildCodes(RecordSet& recordTable, const Node* root, size_t code=0, uint8_t codeLength=0) {
		assert(root);

		if ((root->leaf[0] == nullptr) && (root->leaf[1] == nullptr)) {	// if this node ends the branch
			recordTable.emplace(Record(code, codeLength, root->data));
		}
		else {
			buildCodes(recordTable, root->leaf[0], (code<<1)|0, codeLength+1);
			buildCodes(recordTable, root->leaf[1], (code<<1)|1, codeLength+1);
		}
	}


	/**
	 * Generates an optimal Huffman-code for each symbol based on the frequency table
	 * @param freqTable Look-up frequency table (e.g. freqTable['A'] corresponds to number of occurances of character 'A')
	 * @return
	 */
	static RecordSet encode(const uint32_t freqTable[0x100], uint8_t maxTreeDepth=16) {

		/* Generate unstructurized queue of Huffman nodes, sorted by the weight */
		std::multimap<uint32_t,Node*> huffmanTree;

		for (int i=0; i < 0x100; i++) {
			if (freqTable[i] > 0) {
				huffmanTree.emplace(freqTable[i], new Node(i));
			}
		}

		/* Builds a plain huffman tree */
		while (huffmanTree.size() > 1) {
			/* Merge the first two nodes (which are guaranteed to have the least weights) */
			auto least = huffmanTree.begin();
			auto preLeast = ++huffmanTree.begin();
			Node* merged = new Node(least->second, preLeast->second);

			/* Replaces the affected nodes with a merged one */
			huffmanTree.emplace(least->first + preLeast->first, merged);
			huffmanTree.erase(least);
			huffmanTree.erase(preLeast);
		}

		/* Flatten tree to the given max depth (in bits) */
		const auto & treeRoot = huffmanTree.begin()->second;
		Huffman::flattenTree(treeRoot, maxTreeDepth);

		/* Build final Huffman codes based on inspecting the tree */
		RecordSet recordTable;
		Huffman::buildCodes(recordTable, treeRoot);
		delete treeRoot;

		return recordTable;
	};

	/**
	 * Flattens Huffman tree to a given `maxTreeDepth`; this is required if underlying storage
	 * has a limitation on code length (e.g. 16-bit)
	 */
	static void flattenTree(Node* rootNode, uint8_t maxTreeDepth) {
		assert(rootNode);	// Tree's root node should exist

		/* Generate a map to hold the end nodes (leaf nodes) at each level */
		struct NodeRef { Node* node; Node* parent; };
		const auto sortByParent = [](const std::reference_wrapper<NodeRef>& a, const std::reference_wrapper<NodeRef>& b) {
			if (a.get().parent != b.get().parent) {
				assert(a.get().parent && b.get().parent);
				return a.get().parent->id < b.get().parent->id;
			}
			assert(a.get().node->id != b.get().node->id);
			return a.get().node->id < b.get().node->id;
		};

		std::map<Node*, NodeRef> nodeRefMap;			// map of <Node, reference data>
		std::map<uint8_t, std::set<std::reference_wrapper<NodeRef>, decltype(sortByParent)>> endNodesMap;	// map of <tree depth, reference data>
		{
			const auto traverseTree = [&](Node* node, Node* parent, uint8_t depth, auto& traverseTreeRef) -> void {
				if (node->leaf[0]) {
					traverseTreeRef(node->leaf[0], node, depth + 1, traverseTreeRef);
				}
				if (node->leaf[1]) {
					traverseTreeRef(node->leaf[1], node, depth + 1, traverseTreeRef);
				}
				auto [nodeRefPair, _] = nodeRefMap.emplace(node, NodeRef{node, parent});
				if (!node->leaf[0] && !node->leaf[1]) {
					const auto [_, inserted] = endNodesMap[depth].insert(std::ref(nodeRefPair->second));
					assert(inserted);	// inserting should succeed
				}
			};
			traverseTree(rootNode, nullptr, 0, traverseTree);
		}

		const auto treeDepth = endNodesMap.crbegin()->first;
		if (treeDepth > maxTreeDepth) {
			IO::Log(IO::debug, "Huffman tree is too deep, flattening... (treeDepth=%d)", treeDepth);

			IO::Log(IO::debug, "endNodesMap:");
			for (const auto & [depth, nodes] : endNodesMap) {
				for (const auto & nodeRef : nodes) {
					const auto & [ node, parent ] = nodeRef.get();
					IO::Log(IO::debug, "depth = %d, (Node) data = %X, leafs: [%p, %p]", depth, node->data, node->leaf[0], node->leaf[1]);
				}
			}

			IO::Log(IO::debug, "huffmanTree:");
			const auto traverseTree = [&](Node* node, uint32_t code, uint8_t depth, auto& traverseTreeRef) -> void {
				if (node->leaf[0]) {
					traverseTreeRef(node->leaf[0], (code<<1)|0, depth + 1, traverseTreeRef);
				}
				if (node->leaf[1]) {
					traverseTreeRef(node->leaf[1], (code<<1)|1, depth + 1, traverseTreeRef);
				}
				if (!node->leaf[0] && !node->leaf[1]) {
					IO::Log(IO::debug, "(Node) data = %X, code = %X (%d bits)", node->data, code, depth);
				}
			};
			traverseTree(rootNode, 0, 0, traverseTree);

			/* Source nodes tracking: Get the last 2 nodes from the lowest nodes list */
			for (
				auto lowestLevelRef = std::prev(endNodesMap.end());
				(lowestLevelRef->first > maxTreeDepth) && (lowestLevelRef->second.size() >= 2);
				lowestLevelRef = std::prev(endNodesMap.end())
			) {
				auto lowestTreeNodeB = std::prev(lowestLevelRef->second.end());
				auto lowestTreeNodeA = std::prev(lowestTreeNodeB);
				auto & lowestTreeNodeARef = lowestTreeNodeA->get();
				auto & lowestTreeNodeBRef = lowestTreeNodeB->get();
				IO::Log(IO::debug, "lowestTreeNodeA: %p (NODE) data=%X, depth=%d, parent=%p (NODE) leafs=[%p, %p]",
					lowestTreeNodeARef.node, lowestTreeNodeARef.node->data, lowestLevelRef->first, lowestTreeNodeARef.parent, lowestTreeNodeARef.parent->leaf[0], lowestTreeNodeARef.parent->leaf[1]);
				IO::Log(IO::debug, "lowestTreeNodeB: %p (NODE) data=%X, depth=%d, parent=%p (NODE) leafs=[%p, %p]",
					lowestTreeNodeBRef.node, lowestTreeNodeBRef.node->data, lowestLevelRef->first, lowestTreeNodeBRef.parent, lowestTreeNodeBRef.parent->leaf[0], lowestTreeNodeBRef.parent->leaf[1]);

				/* Sanity check: Last 2 nodes from the "end nodes map" should have the same parent */
				if (lowestTreeNodeARef.parent != lowestTreeNodeBRef.parent) {
					{
						IO::Log(IO::debug, "Pre-exception dump: All end nodes with on the same depth:");
						for (const auto & node : lowestLevelRef->second) {
							IO::Log(IO::debug, "%p (NODE) data=%X, parent=%p (NODE) leafs=[%p, %p]",
								node.get().node, node.get().node->data, node.get().parent, node.get().parent->leaf[0], node.get().parent->leaf[1]
							);
						}
					}
					throw "Internal Huffman tree flattening error: Lowest node pair is corrupted.";
				}

				/* Get the first available node at level "maxTreeDepth - 1" or above */
				/* TODO: Consider moving to the level above instead of `maxTreeDepth - 1`,
				   this may result in slightly compression at the expense of more tree rebalancing steps */
				auto targetLevelRef = std::prev(endNodesMap.upper_bound(maxTreeDepth - 1));
				auto targetEndNode = std::prev(targetLevelRef->second.end());
				auto & targetEndNodeRef = targetEndNode->get();

				IO::Log(IO::debug, "targetEndNodeRef: (NODE) data=%X, depth=%d, parent=%p", targetEndNodeRef.node->data, targetLevelRef->first, lowestTreeNodeBRef.parent);

				/* Detach `lowestTreeNodeA`, `lowestTreeNodeB` from its original parent, attach to `--targetEndNode` */
				Node * lowestTreeParentNode = lowestTreeNodeARef.parent;
				if (targetEndNodeRef.node->leaf[0] || targetEndNodeRef.node->leaf[1]) {
					throw "Internal Huffman tree flattening error: Target end node is corrupted.";
				}
				targetEndNodeRef.node->leaf[0] = lowestTreeParentNode->leaf[0];
				targetEndNodeRef.node->leaf[1] = lowestTreeParentNode->leaf[1];
				lowestTreeParentNode->leaf[0] = nullptr;
				lowestTreeParentNode->leaf[1] = nullptr;

				/* Cycle "data": "targetEndNodeRef" -> "lowestTreeNodeA" -> "lowestTreeNodeB" -> "lowestTreeParentNode" */
				lowestTreeParentNode->data = lowestTreeNodeBRef.node->data;
				lowestTreeNodeBRef.node->data = lowestTreeNodeARef.node->data;
				lowestTreeNodeARef.node->data = targetEndNodeRef.node->data;
				targetEndNodeRef.node->data = 0;

				lowestTreeNodeARef.parent = targetEndNodeRef.node;
				lowestTreeNodeBRef.parent = targetEndNodeRef.node;

				/* `lowestTreeParentNode` itself becomes a new end node */
				auto lowestTreeParentNodeRef = nodeRefMap.find(lowestTreeParentNode);
				if (lowestTreeParentNodeRef == nodeRefMap.end()) {
					throw "Internal Huffman tree flattening error: Failed to locate parent of a current lowest parent node.";
				}

				/* Rebalance `endNodesMap` tree: Relocate `lowestTreeNodeA` and `lowestTreeNodeA` to a different level/depth, remove `targetEndNode` */
				endNodesMap[lowestLevelRef->first - 1].emplace(std::ref(lowestTreeParentNodeRef->second));
				endNodesMap[targetLevelRef->first + 1].emplace(std::ref(lowestTreeNodeARef));
				endNodesMap[targetLevelRef->first + 1].emplace(std::ref(lowestTreeNodeBRef));

				lowestLevelRef->second.erase(lowestTreeNodeA);
				lowestLevelRef->second.erase(lowestTreeNodeB);
				if (lowestLevelRef->second.empty()) {
					endNodesMap.erase(lowestLevelRef);
				}
				targetLevelRef->second.erase(targetEndNode);
				if (targetLevelRef->second.empty()) {
					endNodesMap.erase(targetLevelRef);
				}
			};

			IO::Log(IO::debug, "huffmanTree: (after flatenning)");
			traverseTree(rootNode, 0, 0, traverseTree);
		}
	}
	
};
