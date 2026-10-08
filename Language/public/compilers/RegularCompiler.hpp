
#pragma once

#include "../grammars/RegularGrammar.hpp"
#include "../automatas/FinateStateAutomata.hpp"

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

		void compile(Graph& graph, const tAlphabetType* regex, tStateType state);
		void compile(Graph& aGraph, const Grammar& grammar);

	private:
		Node compileUtil(const Grammar::Node* astNode, tStateType state);
		Node compileVal(Grammar::ValueNode* val, Vertex* aLeft = nullptr, Vertex* aRight = nullptr);
		Node compileAlternation(const Grammar::AlternationNode* alt, Vertex* aLeft = nullptr, Vertex* aRight = nullptr);
		Node compileAny(const Grammar::AnyNode*, Vertex* aLeft = nullptr, Vertex* aRight = nullptr);
		Node compileRepeat(const Grammar::RepetitionNode* repeat, Vertex* aLeft = nullptr, Vertex* aRight = nullptr);
		Node compileIf(const Grammar::IfNode* ifNode, Vertex* aLeft = nullptr, Vertex* aRight = nullptr);
		Node compileClass(const Grammar::ClassNode* node, Vertex* aLeft = nullptr, Vertex* aRight = nullptr);
		Node compileCompound(const Grammar::CompoundNode* compound, Vertex* aLeft = nullptr, Vertex* aRight = nullptr);
		Node compileNode(const Grammar::Node* node, Vertex* aLeft = nullptr, Vertex* aRight = nullptr);

		void transitionAny(Vertex* from, Vertex* to, bool consumes = false);
		void transitionEpsilon(Vertex* from, Vertex* to);
		void transitionVal(Vertex* from, Vertex* to, tAlphabetType val);
		void transitionRange(Vertex* from, Vertex* to, IterRange<ualni> range, bool exclude);

		Vertex* addVertex();
	};
}

#include "compilers/RegularCompiler.ipp"
