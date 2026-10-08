namespace tp {
	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	void RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::compile(Graph& graph, const tAlphabetType* regex, tStateType state) {
		mGraph = &graph;
		compileUtil(regex, state);
	}

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	void RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::compile(Graph& aGraph, const Grammar& grammar) {
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

			transitionEpsilon(left, node.left);
			transitionEpsilon(node.right, right);

			idx++;
		}

		mGraph->setStartState(left);
	}

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	auto RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::compileUtil(const Grammar::Node* astNode, tStateType state) -> Node {

		auto node = compileNode(astNode, nullptr, nullptr);

		node.right->setValue(state);
		node.right->setAcceptance(true);

		mGraph->setStartState(node.left);

		return node;
	}

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	auto RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::compileVal(Grammar::ValueNode* val, Vertex* aLeft, Vertex* aRight) -> Node {
		auto left = aLeft ? aLeft : addVertex();
		auto right = aRight ? aRight : addVertex();
		transitionVal(left, right, val->mVal);
		return { left, right };
	}

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	auto RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::compileAlternation(const Grammar::AlternationNode* alt, Vertex* aLeft, Vertex* aRight) -> Node {
		auto first_node = compileNode(alt->mFirst, aLeft, aRight);
		auto second_node = compileNode(alt->mSecond);
		transitionEpsilon(first_node.left, second_node.left);
		transitionEpsilon(second_node.right, first_node.right);
		return first_node;
	}

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	auto RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::compileAny(const Grammar::AnyNode*, Vertex* aLeft, Vertex* aRight) -> Node {
		auto left = aLeft ? aLeft : addVertex();
		auto right = aRight ? aRight : addVertex();
		transitionAny(left, right, true);
		return { left, right };
	}

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	auto RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::compileRepeat(const Grammar::RepetitionNode* repeat, Vertex* aLeft, Vertex* aRight) -> Node {
		if (repeat->mPlus) {
			auto middle = addVertex();

			auto left_node = compileNode(repeat->mNode, aLeft, middle);

			auto right_node = compileNode(repeat->mNode, middle, aRight);
			transitionEpsilon(right_node.right, right_node.left);
			transitionEpsilon(right_node.left, right_node.right);

			return { left_node.left, right_node.right };
		} else {
			auto node = compileNode(repeat->mNode, aLeft, aRight);
			transitionEpsilon(node.right, node.left);
			transitionEpsilon(node.left, node.right);
			return node;
		}
	}

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	auto RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::compileIf(const Grammar::IfNode* ifNode, Vertex* aLeft, Vertex* aRight) -> Node {
		auto node = compileNode(ifNode->mNode, aLeft, aRight);
		transitionEpsilon(node.left, node.right);
		return node;
	}

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	auto RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::compileClass(const Grammar::ClassNode* node, Vertex* aLeft, Vertex* aRight) -> Node {
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
			transitionEpsilon(middle, right);
		}
		return { left, right };
	}

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	auto RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::compileCompound(const Grammar::CompoundNode* compound, Vertex* aLeft, Vertex* aRight) -> Node {
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

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	auto RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::compileNode(const Grammar::Node* node, Vertex* aLeft, Vertex* aRight) -> Node {
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

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	void RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::transitionAny(Vertex* from, Vertex* to, bool consumes) {
		for (auto symbol : IterRange<ualni>(tMinSymbol, tMaxSymbol)) {
			transitionVal(from, to, symbol);
		}
	}

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	void RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::transitionEpsilon(Vertex* from, Vertex* to) { mGraph->addEpsilonTransition(from, to); }

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	void RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::transitionVal(Vertex* from, Vertex* to, tAlphabetType val) { mGraph->addTransition(from, to, val); }

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	void RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::transitionRange(Vertex* from, Vertex* to, IterRange<ualni> range, bool exclude) {
		if (exclude) {
			IterRange<ualni> first = { tMinSymbol, range.mBegin };
			IterRange<ualni> second = { range.mEnd, tMaxSymbol };

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

	template <typename tAlphabetType, typename tStateType, tStateType tInTransition, ualni tMinSymbol, ualni tMaxSymbol>
	auto RegularCompiler<tAlphabetType, tStateType, tInTransition, tMinSymbol, tMaxSymbol>::addVertex() -> Vertex* { return mGraph->addState(tInTransition, false); }
}