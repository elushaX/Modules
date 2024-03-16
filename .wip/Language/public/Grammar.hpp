
#pragma once

#include "Buffer.hpp"
#include "LanguageCommon.hpp"
#include "Map.hpp"
#include "Strings.hpp"

namespace tp {

	class ContextFreeGrammar {
	public:
		struct Arg {
			friend class Rule;

		public:
			Arg() = default;
			explicit Arg(const String& id, bool terminal = true, bool epsilon = false);

		public:
			bool operator==(const Arg& in) const;
			[[nodiscard]] const String& getId() const;
			[[nodiscard]] bool isTerminal() const { return mIsTerminal; }
			[[nodiscard]] bool isEpsilon() const { return mIsEpsilon; }

		private:
			String mId;
			bool mIsTerminal = false;
			bool mIsEpsilon = false;
		};

		class Rule {
		public:
			Rule() = default;
			Rule(const String& id, const InitialierList<Arg>& args);

		public:
			bool operator==(const Rule& in) const;
			[[nodiscard]] bool isProductive() const;
			[[nodiscard]] const String& getId() const { return mId; }
			[[nodiscard]] const Buffer<Arg>* getArgs() const { return &mArgs; }

		private:
			String mId;
			Buffer<Arg> mArgs;
		};

	public:
		ContextFreeGrammar() = default;

	public:
		void addRule(const Rule& rule);
		void addRule(const String& id, const InitialierList<Arg>& args);
		void setStart(const String& startRule);
		[[nodiscard]] const Buffer<Rule>* getRules() const { return &mRules; }
		[[nodiscard]] const String& getStartTerminal() const { return mStartTerminal; }

	public:
		Buffer<Rule> mRules;
		String mStartTerminal;
		bool mIsLooped = false;
	};

	template <typename tAlphabetType, typename tTokType>
	class RegularGrammar {
	public:
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

		public:
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

		public:
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

		public:
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

		public:
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

		public:
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
				mRanges = ranges;
			}

			~ClassNode() override { mRanges.removeAll(); }

		public:
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
		const Node* ranges(const Buffer<Range<tAlphabetType>>& ranges, bool exclude = false) {
			return new ClassNode(ranges, exclude);
		}

	public:
		Buffer<Pair<const Node*, tTokType>> mRules;
	};
}
