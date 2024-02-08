
#pragma once

#include "Grammar.hpp"
#include "Automata.hpp"

namespace tp {

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	class RegularCompiler {

		typedef FiniteStateAutomation<tAlphabetType, tStateType> Graph;
		typedef typename Graph::State Vertex;
		typedef RegularGrammar<tAlphabetType, tStateType> Grammar;

		struct Node {
			Vertex* left = nullptr;
			Vertex* right = nullptr;
		};

	private:
		Graph* mGraph = nullptr;

	public:
		struct CompileError {
			uhalni mRuleIndex = 0;
			tStateType mRuleState;
			const char* description = nullptr;
			[[nodiscard]] bool isError() const { return description; }
		};

		CompileError mError;

		void compile(Graph& graph, const tAlphabetType* regex, tStateType state) {
			mGraph = &graph;
			compileUtil(regex, state);
		}

		void compile(Graph& aGraph, const Grammar& grammar) {
			mGraph = &aGraph;

			auto left = addVertex();
			auto right = addVertex();

			halni idx = 0;
			for (auto rule : grammar.mRules) {

				auto node = compileUtil(rule.data().first, rule.data().second);

				if (!(node.left && node.right)) {
					mError.mRuleIndex = idx;
					return;
				}

				transitionAny(left, node.left);
				transitionAny(node.right, right);

				idx++;
			}

			mGraph->setStartState(left);
		}

	private:
		Node compileUtil(const Grammar::Node* astNode, tStateType state) {

			auto node = compileNode(astNode, nullptr, nullptr);

			node.right->setValue(state);
			node.right->setAcceptance(true);

			mGraph->setStartState(node.left);

			return node;
		}

		Node compileVal(Grammar::ValueNode* val, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
			auto left = aLeft ? aLeft : addVertex();
			auto right = aRight ? aRight : addVertex();
			transitionVal(left, right, val->mVal);
			return { left, right };
		}

		Node compileAlternation(const Grammar::AlternationNode* alt, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
			auto first_node = compileNode(alt->mFirst, aLeft, aRight);
			auto second_node = compileNode(alt->mSecond);
			transitionAny(first_node.left, second_node.left);
			transitionAny(second_node.right, first_node.right);
			return first_node;
		}

		Node compileAny(const Grammar::AnyNode*, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
			auto left = aLeft ? aLeft : addVertex();
			auto right = aRight ? aRight : addVertex();
			transitionAny(left, right, true);
			return { left, right };
		}

		Node compileRepeat(const Grammar::RepetitionNode* repeat, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
			if (repeat->mPlus) {
				auto middle = addVertex();

				auto left_node = compileNode(repeat->mNode, aLeft, middle);

				auto right_node = compileNode(repeat->mNode, middle, aRight);
				transitionAny(right_node.right, right_node.left);
				transitionAny(right_node.left, right_node.right);

				return { left_node.left, right_node.right };
			} else {
				auto node = compileNode(repeat->mNode, aLeft, aRight);
				transitionAny(node.right, node.left);
				transitionAny(node.left, node.right);
				return node;
			}
		}

		Node compileIf(const Grammar::IfNode* ifNode, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
			auto node = compileNode(ifNode->mNode, aLeft, aRight);
			transitionAny(node.left, node.right);
			return node;
		}

		Node compileClass(const Grammar::ClassNode* node, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
			auto left = aLeft ? aLeft : addVertex();
			auto right = aRight ? aRight : addVertex();

			if (node->mRanges.size() == 1) {
				auto const& range = node->mRanges.first();
				transitionRange(left, right, { ualni(range.mBegin), ualni(range.mEnd) }, node->mExclude);
				return { left, right };
			}

			for (auto range : node->mRanges) {
				auto middle = addVertex();
				transitionRange(left, middle, { ualni(range->mBegin), ualni(range->mEnd) }, node->mExclude);
				transitionAny(middle, right);
			}
			return { left, right };
		}

		Node compileCompound(const Grammar::CompoundNode* compound, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
			Vertex* left = nullptr;
			Vertex* rigth = nullptr;

			ualni idx = 0;
			for (auto child : compound->mSequence) {
				auto pass_left = idx == 0 ? aLeft : rigth;
				auto pass_right = idx == compound->mSequence.size() - 1 ? aRight : nullptr;
				auto node = compileNode(child.data(), pass_left, pass_right);
				if (!left) left = node.left;
				rigth = node.right;
				idx++;
			}

			return { left, rigth };
		}

		Node compileNode(const Grammar::Node* node, Vertex* aLeft = nullptr, Vertex* aRight = nullptr) {
			switch (node->mType) {
				case Grammar::Node::CLASS: return compileClass((typename Grammar::ClassNode*) node, aLeft, aRight);
				case Grammar::Node::COMPOUND: return compileCompound((typename Grammar::CompoundNode*) node, aLeft, aRight);
				case Grammar::Node::IF: return compileIf((typename Grammar::IfNode*) node, aLeft, aRight);
				case Grammar::Node::REPEAT: return compileRepeat((typename Grammar::RepetitionNode*) node, aLeft, aRight);
				case Grammar::Node::ANY: return compileAny((typename Grammar::AnyNode*) node, aLeft, aRight);
				case Grammar::Node::OR: return compileAlternation((typename Grammar::AlternationNode*) node, aLeft, aRight);
				case Grammar::Node::VAL: return compileVal((typename Grammar::ValueNode*) node, aLeft, aRight);
				case Grammar::Node::NONE: break;
			}
			ASSERT(0)
			return {};
		}

		void transitionAny(Vertex* from, Vertex* to, bool consumes = false) {
			for (auto symbol : Range<ualni>(tMinSymbol, tMaxSymbol)) {
				transitionVal(from, to, symbol);
			}
		}

		void transitionVal(Vertex* from, Vertex* to, tAlphabetType val) { mGraph->addTransition(from, to, val); }

		void transitionRange(Vertex* from, Vertex* to, Range<ualni> range, bool exclude) {
			if (exclude) {
				Range<ualni> first = { tMinSymbol, range.mBegin - 1 };
				Range<ualni> second = { range.mEnd + 1, tMaxSymbol };

				if (first.valid()) {
					for (auto symbol : first) {
						transitionVal(from, to, symbol);
					}
				}

				if (second.valid()) {
					for (auto symbol : second) {
						transitionVal(from, to, symbol);
					}
				}

			} else {
				for (auto symbol : range) {
					transitionVal(from, to, symbol);
				}
			}
		}

		Vertex* addVertex() { return mGraph->addState(tInTransition, false); }
	};
}
