
#pragma once

#include "Buffer.hpp"
#include "LanguageCommon.hpp"
#include "Utils.hpp"

namespace tp {

	class ContextFreeGrammar {};

	template <typename tAlphabetType, typename tTokType>
	class RegularGrammar {

		struct Node {
			enum Type {
				NONE,
				ANY,
				OR,
				IF,
				CLASS,
				COMPOUND,
				REPEAT,
				VAL,
			} mType = NONE;

			explicit Node(Type type) :
				mType(type) {}

			virtual ~Node() = default;
		};

		class ValueNode : public Node {
		public:
			explicit ValueNode(tAlphabetType val) :
				mVal(val),
				Node(Node::VAL) {}

			~ValueNode() override = default;

		private:
			tAlphabetType mVal;
		};

		class CompoundNode : public Node {
		public:
			CompoundNode() :
				Node(Node::COMPOUND) {}

			CompoundNode(const InitialierList<const Node*>& nodes) :
				Node(Node::COMPOUND) {
				mSequence = nodes;
			}

			~CompoundNode() override {
				for (auto iter : mSequence) {
					delete iter.data();
				}
				mSequence.clear();
			}

		private:
			Buffer<const Node*> mSequence;
		};

		class AlternationNode : public Node {
		public:
			AlternationNode() :
				Node(Node::OR) {}

			AlternationNode(const Node* a, const Node* b) :
				Node(Node::OR) {
				mFirst = a;
				mSecond = b;
			}

			~AlternationNode() override {
				delete mFirst;
				delete mSecond;
			}

		private:
			const Node* mFirst = nullptr;
			const Node* mSecond = nullptr;
		};

		class IfNode : public Node {
		public:
			IfNode() :
				Node(Node::IF) {}

			explicit IfNode(const Node* a) :
				Node(Node::IF) {
				mNode = a;
			}

			~IfNode() override { delete mNode; }

		private:
			const Node* mNode = nullptr;
		};

		class AnyNode : public Node {
		public:
			AnyNode() :
				Node(Node::ANY) {}

			~AnyNode() override = default;
		};

		class RepetitionNode : public Node {
		public:
			RepetitionNode() :
				Node(Node::REPEAT) {}

			explicit RepetitionNode(const Node* rep, bool plus = false) :
				Node(Node::REPEAT) {
				mNode = rep;
				mPlus = plus;
			}

			~RepetitionNode() override { delete mNode; }

		private:
			Node* mNode = nullptr;
			bool mPlus = false;
		};

		class ClassNode : public Node {
		public:
			ClassNode() :
				Node(Node::CLASS) {}

			explicit ClassNode(const Buffer<Range<tAlphabetType>>& ranges, bool exclude = false) :
				Node(Node::CLASS) {
				mExclude = exclude;
			}

			~ClassNode() override { mRanges.removeAll(); }

		private:
			Buffer<Range<tAlphabetType>> mRanges;
			bool mExclude = false;
		};

	public:
		RegularGrammar() = default;

		~RegularGrammar() {
			for (auto rule : mRules) {
				delete rule->t1;
			}
		}

		void addRule(const Node* node, tTokType id) { mRules.append({ node, id }); }

	public:
		const Node* seq(const InitialierList<const Node*>& nodes) { return new CompoundNode(nodes); }
		const Node* val(tAlphabetType in) { return new ValueNode(in); }
		const Node* alt(const Node* a, const Node* b) { return new AlternationNode(a, b); }
		const Node* may(const Node* a) { return new IfNode(a); }
		const Node* any() { return new AnyNode(); }
		const Node* rep(const Node* rep, bool plus = false) { return new RepetitionNode(rep, plus); }
		const Node* ranges(const Buffer<Range<tAlphabetType>>& ranges, bool exclude = false) { return new ClassNode(ranges, exclude); }

	private:
		Buffer<Pair<const Node*, tTokType>> mRules;
	};
}
