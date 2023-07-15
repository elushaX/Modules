
#pragma once

#include "Strings.hpp"
#include "Buffer.hpp"
#include "Tree.hpp"

namespace tp {

	class PiecesDS {

		struct Input {
			const Char* mInput = nullptr;
			Index mLen = 0;
			void getLineOffsets(Buffer<Index>& out);
		};

		struct Pointer {
			LineId mLine;
			ColumnId mCol;

			Pointer();
			Pointer(LineId aLine, ColumnId aCol);
			Pointer(const Input& aInput);
			void operator+=(Char achar);
			void operator+=(const Pointer& ap);
			Pointer operator-(const Pointer in) const;
			Pointer operator+(const Pointer& ap) const;
			bool operator==(const Pointer& in) const;
			bool operator>(const Pointer& in) const;
			bool operator>=(const Pointer& in) const;
		};

		struct Piece {
			bool mAddedBuff = false;
			Pointer mStart;
			Pointer mEnd;

			Pointer diffPtr() const;
			LineId nLines() const;
		};

	public:

		// Tree Node : Based on AvlNodeBlueprint
		struct Node {

			Node* mLeft = nullptr;
			Node* mRight = nullptr;
			Node* mParent = nullptr;
			halni mHeight = 0;

			Pointer mPtrLeft;
			Pointer mPtrRight;
			Pointer mDiff;

			Node* mNext = nullptr;
			Node* mPrev = nullptr;
			Piece* mPiece = nullptr;

			static bool mInsertionOperation;

			Node() {}
			Node(Pointer aKey) {
				//mVal = aKey;
			}

			bool disentRight(Pointer aKey) const {
				return aKey > mPtrLeft;
			}

			bool disentLeft(Pointer aKey) const {
				return mPtrLeft > aKey;
			}

			bool exactNode(Pointer aKey) const {
				if (mInsertionOperation) {
					return aKey > mPtrLeft && mPtrLeft + mDiff > aKey;
				}
				return aKey >= mPtrLeft && mPtrLeft + mDiff > aKey;
			}

			Pointer getFindKey(Node* node = nullptr) const {
				// node must be ancestor of this node

				Pointer out = mPtrLeft;

				if (node == this) {
					return out;
				}

				auto iter = this;
				while (iter->mParent != node) {
					if (iter == iter->mParent->mRight) {
						auto left_sub_tree = iter->mParent->mPtrLeft + iter->mParent->mDiff;
						out = left_sub_tree + out;
					}
					iter = iter->mParent;
				}

				if (node) {
					if (iter == iter->mParent->mRight) {
						auto left_sub_tree = iter->mParent->mPtrLeft + iter->mParent->mDiff;
						out = left_sub_tree + out;
					}
				}
				return out;
			}

			void copyData(const Node& in) {
				mPiece = in.mPiece;
				mNext = in.mNext;
				mPrev = in.mPrev;

				if (mPrev) {
					mPrev->mNext = this;
				}

				if (mNext) {
					mNext->mPrev = this;
				}
			}

			Pointer keyInRightSubtree(Pointer key) const { return key - (mPtrLeft + mDiff); }
			Pointer keyInLeftSubtree(Pointer key) const { return key; }

			void updateTreeCahceCallBack() {
				DEBUG_ASSERT(mPiece);
				mDiff = mPiece->diffPtr();
				if (mLeft) {
					mPtrLeft = mLeft->sum();
				}
				else {
					mPtrLeft = { 0, 0 };
				}

				if (mRight) {
					mPtrRight = mRight->sum();
				}
				else {
					mPtrRight = { 0, 0 };
				}
			}

			Pointer sum() const {
				return (mPtrLeft + mDiff) + mPtrRight;
			}
		};

	private:

		AvlTree<Node, Pointer> mTree;

	public:

		PiecesDS() {
		}

		Node* find(Pointer ptr) const  {
			return mTree.find(ptr);
		}

		Node* minNode(Node* node) const {
			return mTree.minNode(node);
		}

		Node* maxNode(Node* node) const {
			return mTree.maxNode(node);
		}

		Node* head() const {
			return mTree.head();
		}

		void insert(const Node& node, Pointer ptr) {
			Node::mInsertionOperation = true;
			mTree.insert(ptr, node);
			Node::mInsertionOperation = false;
		}

		Node* insertAfter(Node* node, Piece piece) {
			auto key = node ? (node->getFindKey() + node->mDiff) : Pointer{ 0, 0 };
			auto new_node = Node();
			new_node.mPiece = new Piece(piece);

			insert(new_node, key);

#ifdef _DEBUG
			auto err_node = mTree.isInvalid(mTree.head());
			DBG_BREAK(err_node);
#endif

			auto out = mTree.find(key);

			Node* next = out->mRight;
			Node* prev = out->mLeft;

			if (!(next || prev) && out->mParent) {
				if (out->mParent->mLeft == out) {
					next = out->mParent;
				}
				else {
					prev = out->mParent;
				}
			}

			if (next) {
				out->mNext = next;
				out->mPrev = next->mPrev;
				next->mPrev = out;
				if (out->mPrev) out->mPrev->mNext = out;
			}
			else if (prev) {
				out->mPrev = prev;
				out->mNext = prev->mNext;
				prev->mNext = out;
				if (out->mNext) out->mNext->mPrev = out;
			}

			return out;
		}

		void remove(Node* aNode) {
			auto key = aNode->getFindKey();

			if (aNode->mPrev) {
				aNode->mPrev->mNext = aNode->mNext;
			}

			if (aNode->mNext) {
				aNode->mNext->mPrev = aNode->mPrev;
			}

			auto del = aNode->mPiece;
			mTree.remove(key);
			delete del;

#ifdef _DEBUG
			auto err_node = mTree.isInvalid(mTree.head());
			DBG_BREAK(err_node);
#endif
		}

		Node* first() const {
			return mTree.minNode(mTree.head());
		}

		Pointer ptrSum() const {
			if (mTree.head()) {
				return mTree.head()->sum();
			}
			return { 0, 0 };
		}

		halni nPieces() const {
			return mTree.size();
		}

		void update_cache(Node* node) {
			if (node) {
				node->updateTreeCahceCallBack();
				update_cache(node->mParent);
			}
		}

		void undo() {}
		void redo() {}
		void pushUndoPoint() {}
	};

	struct AddBuff {
		Buffer<Char> mBuff;
		Buffer<Index> mLineOffsets;
		Index mColumn = 0;

		AddBuff();
		void append(Input aInput);
		Pointer endPtr();
	};

	struct OrigBuff {
		bool mOwnsInput = false;
		Input mOriginal;
		Buffer<Index> mLineOffsets;

		Pointer mPtrStart;
		Pointer mPtrEnd;

		// does not owns the original input by default
		OrigBuff(Input aOriginal);

		void makeOwnOriginalInput();

		~OrigBuff();
	};

	struct PieceCrs {
		Pointer piece_ptr;
		Pointer offset;
		Pointer ptr;
		PiecesDS::Node* piece = nullptr;
	};

	class TextEditor {
		friend class Iterator;

		class Iterator {
			friend TextEditor;

			const TextEditor* mSelf;
			PiecesDS::Node* mNode = nullptr;
			Pointer mOffset = { 0, 0 };
			Pointer mPtr = { 0, 0 };
			Char mVal = 0;

			Iterator(const TextEditor* mSelf);
			bool inputLeft();

		public:

			Char character();
			void operator++();
			bool operator!=(alni const& iter);
			bool operator==(alni const& iter);
			Char operator->();
			const Iterator& operator*();
		};

	public:

		// pieces of strings that construct the text
		PiecesDS mPieces;

		// string buffers to wich pieces can reffer
		OrigBuff mOrigBuff;
		AddBuff mAddBuff;

		// active position on the text
		Pointer mCursor;
		Pointer mCursorEnd;

		Pointer mLastInsertionCrs;
	//private:

		[[nodiscard]] PieceCrs findPiece(Pointer ptr) const;
		[[nodiscard]] Pointer prevPiecePtr(PiecesDS::Node* piece_node, Pointer piece_pointer) const;
		[[nodiscard]] Pointer nextPiecePtr(PiecesDS::Node* piece_node, Pointer piece_pointer) const;

		[[nodiscard]] const Char* getBuffLine(const Piece*, LineId) const;
		[[nodiscard]] const Char* getPieceLine(const Piece*, LineId) const;
		[[nodiscard]] PiecesDS::Node* splitPiece(PiecesDS::Node* piece, Pointer split_ptr);
		[[nodiscard]] LineId getPieceLineLen(Piece* aPiece, LineId line) const;

	public:
		enum Dir : uint1 { LEFT, RIGHT, UP, DOWN };
		enum DirUnit : uint1 { CHAR, WORD, TOK, LINE_END, LINE_START };

	public:

		TextEditor();
		explicit TextEditor(Input original);

		[[nodiscard]] Index mNLines() const;
		void clampPtr(Pointer& ptr) const;
		[[nodiscard]] Pointer getCursor() const;
		[[nodiscard]] Pointer getCursorEnd() const;
		void setCursor(Pointer ptr);
		void setCursorEnd(Pointer ptr);

		void insert(Input aInput);

		void remove(bool only_selection = false, Dir dir = LEFT, DirUnit unit = CHAR);

		void undo();
		void redo();

		Iterator begin();
		alni end();

		~TextEditor();
	};
}