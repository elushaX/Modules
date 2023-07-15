
#pragma once

#include "List.hpp"
#include "Buffer.hpp"
#include "Buffer2D.hpp"
#include "Utils.hpp"
#include "Map.hpp"

namespace tp {

	extern ModuleManifest gModuleTokenizer;

	template <typename tAlphabetType, typename tStateType, tStateType tNoStateVal, tStateType tFailedStateVal>
	class TransitionMatrix;

	template <typename tAlphabetType, typename tStateType, tStateType tNoStateVal, tStateType tFailedStateVal>
	class DFA;

	// Non-Deterministic Finite-State Automata
	template <typename tAlphabetType, typename tStateType, tStateType tNoStateVal, tStateType tFailedStateVal>
	class NFA {
		static_assert(TypeTraits<tAlphabetType>::isIntegral, "tAlphabetType must be enumerable.");

	public:
		struct Vertex;

	private:
		struct Edge {
			Vertex* mVertex = nullptr;
			bool mConsumesSymbol = false;
			Range<tAlphabetType> mAcceptingRange;
			bool mAcceptsAll = false;
			bool mExclude = false;

			bool isTransition(const tAlphabetType& symbol) {
				if (symbol == 0) return false;
				if (!mConsumesSymbol || mAcceptsAll) return true;
				bool const in_range = (symbol >= mAcceptingRange.mBegin && symbol <= mAcceptingRange.mEnd);
				return in_range != mExclude;
			}
		};

	public:
		struct Vertex {
			List<Edge> edges;
			tStateType termination_state = tNoStateVal;
			#ifdef ENV_BUILD_DEBUG
				ualni debug_idx = 0;
			#endif
			ualni flag = 0;
		};

	private:
		friend DFA<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal>;

		List<Vertex> mVertices;
		Vertex* mStart = nullptr;

	public:
		NFA() = default;

		Vertex* addVertex() {
			auto node = mVertices.newNode();
			#ifdef ENV_BUILD_DEBUG
				node->data.debug_idx = mVertices.length() + 1;
			#endif
			mVertices.pushBack(node);
			return &node->data;
		}

		void addTransition(Vertex* from, Vertex* to, Range<tAlphabetType> range, bool consumes, bool accepts_all, bool exclude) {
			Edge edge; 
			edge.mVertex = to;
			edge.mConsumesSymbol = consumes;
			edge.mAcceptingRange = range;
			edge.mExclude = exclude;
			edge.mAcceptsAll = accepts_all;
			from->edges.pushBack(edge);
		}

		void setStartVertex(Vertex* start) {
			mStart = start;
		}

		[[nodiscard]] Vertex* getStartVertex() const {
			return mStart;
		}

		void setVertexState(Vertex* vertex, tStateType state) {
			vertex->termination_state = state;
		}

		[[nodiscard]] bool isValid() const {
			if (!mStart) {
				return false;
			}
			return true;
		}

		Range<tAlphabetType> getAlphabetRange() const {
			tAlphabetType start = 0, end = 0;
			Range<tAlphabetType> all_range = { std::numeric_limits<tAlphabetType>::min(), std::numeric_limits<tAlphabetType>::max() };
			bool first = true;

			for (auto vertex : mVertices) {
				for (auto edge : vertex.data().edges) {
					if (!edge.data().mConsumesSymbol) continue;
					auto const& tran_range = edge.data().mAcceptingRange;
					if (edge.data().mAcceptsAll || edge.data().mExclude) return all_range;
					if (tran_range.mBegin < start || first) start = tran_range.mBegin;
					if (tran_range.mEnd > end || first) end = tran_range.mEnd;
					first = false;
				}
			}

			return Range<tAlphabetType>( start, end + 1 );
		}

		// vertices that are reachable from initial set with no input consumption (E-transitions)
		// does not include initial set
		void closure(const List<Vertex*>& set, List<Vertex*>& closure, ualni unique_call_id) {
			List<Vertex*> marked;
			marked = set;

			while (marked.length()) {
				auto first = marked.first()->data;
				if (first->flag != unique_call_id) {
					first->flag = unique_call_id;
					closure.pushBack(first);
				}
				for (auto edge : first->edges) {
					if (!edge.data().mConsumesSymbol && edge.data().mVertex->flag != unique_call_id) {
						marked.pushBack(edge.data().mVertex);
					}
				}
				marked.popFront();
			}
		}

		// vertices that are reachable from initial set with symbol transition
		void move(const List<Vertex*>& set, List<Vertex*>& reachable, tAlphabetType symbol, ualni unique_call_id) {
			for (auto vertex : set) {
				for (auto edge : vertex->edges) {
					if (!edge.data().mConsumesSymbol) continue;
					bool transition = edge.data().isTransition(symbol);
					if (transition && edge.data().mVertex->flag != unique_call_id) {
						edge.data().mVertex->flag = unique_call_id;
						reachable.pushBack(edge.data().mVertex);
					}
				}
			}
		}
	};

	// Deterministic Finite-State Automata
	template <typename tAlphabetType, typename tStateType, tStateType tNoStateVal, tStateType tFailedStateVal>
	class DFA {
		static_assert(TypeTraits<tAlphabetType>::isIntegral, "tAlphabetType must be enumerable.");
		friend TransitionMatrix<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal>;

		struct Vertex {
			struct Edge {
				Vertex* vertex = nullptr;
				tAlphabetType transition_code = nullptr;
			};

			List<Edge> edges;
			tStateType termination_state = tNoStateVal;
			bool marked = false;
		};

		List<Vertex> mVertices;
		Vertex* mStart = nullptr;
		const Vertex* mIter = nullptr;
		Range<tAlphabetType> mAlphabetRange;
		bool mTrapState = false;

		typedef typename NFA<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal>::Vertex NState;

	public:

		explicit DFA(NFA<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal>& nfa) {
			if (!nfa.isValid()) {
				return;
			}
			
			mAlphabetRange = nfa.getAlphabetRange();

			struct DStateKey {
				const List<NState*>* nStates;
				bool operator==(const DStateKey& in) const {
					if (nStates->length() != in.nStates->length()) {
						return false;
					}
					// FIXME : make linear time
					for (auto state : *nStates) {
						bool found = false;
						for (auto in_state : *in.nStates) {
							if (state.data() == in_state.data()) {
								found = true;
								break;
							}
						}

						if (!found) {
							return false;
						}
					}

					return true;
				}

				static ualni dStateHashFunc(DStateKey key) {
					alni out = 0;
					for (auto state : *key.nStates) {
						out += alni(state.data());
					}
					return out;
				};
			};

			// all NFA states that are reachable from initial DFA State for specific symbol in alphabet
			struct DState {

				struct DTransition {
					DState* state = nullptr;
					tAlphabetType accepting_code;
				};

				List<NState*> nStates;
				List<DTransition> transitions;

				Vertex* dVertex = nullptr; // relevant DFA vertex

				#ifdef ENV_BUILD_DEBUG
				ualni debug_idx = 0;
				#endif
			};

			// includes closure of NFA start state by definition
			auto start_state = new DState();
			nfa.closure({ nfa.getStartVertex() }, start_state->nStates, ualni(start_state));

			Map<DStateKey, DState*, DefaultAllocator, DStateKey::dStateHashFunc, 256> dStates;

			dStates.put({ &start_state->nStates }, start_state);

			List<DState*> working_set = { start_state };

			// while there is items to work with
			auto currentDState = working_set.first();
			while (currentDState) {

				// check all possible transitions for any symbol
				for (auto symbol : mAlphabetRange) {

					List<NState*> reachableNStates;

					nfa.move(currentDState->data->nStates, reachableNStates, symbol, ualni(currentDState + symbol));
					nfa.closure(reachableNStates, reachableNStates, ualni(currentDState + symbol));

					if (!reachableNStates.length()) {
						continue;
					}

					DState* targetDState = nullptr;

					// check if set of all reachable NFA states already forms existing DFA state
					auto idx = dStates.presents({ &reachableNStates});
					if (idx) {
						targetDState = dStates.getSlotVal(idx);
					}
					
					if (!targetDState) {
						// register new DFA state
						targetDState = new DState();

						#ifdef ENV_BUILD_DEBUG
							targetDState->debug_idx = dStates.size();
						#endif

						targetDState->nStates = reachableNStates;

						// append to working stack
						working_set.pushBack(targetDState);
						dStates.put({ &targetDState->nStates }, targetDState);
					}

					// add transition to DFA state
					currentDState->data->transitions.pushBack({targetDState, symbol });
				}

				working_set.popFront();
				currentDState = working_set.first();
			}

			// create own vertices
			for (auto node : dStates) {
				tStateType state = tNoStateVal;
				for (auto iter : node->val->nStates) {
					if (iter->termination_state != tNoStateVal) {
						state = iter->termination_state;
						break;
					}
				}
				node->val->dVertex = addVertex(state);
			}

			// connect all vertices
			for (auto node : dStates) {
				for (auto edge : node->val->transitions) {
					addTransition(node->val->dVertex, edge.data().state->dVertex, edge.data().accepting_code);
				}
			}

			// set the starting vertex
			mStart = start_state->dVertex;

			// cleanup
			for (auto node : dStates) {
				delete node->val;
			}

			collapseEquivalentVertices();
			mAlphabetRange = getAlphabetRange();
		}

		tStateType move(tAlphabetType symbol) {
			if (mTrapState || !mIter) {
				return tNoStateVal;
			}

			for (auto edge : mIter->edges) {
				if (edge.data().transition_code == symbol) {
					mIter = edge.data().vertex;
					return mIter->termination_state;
				}
			}

			mTrapState = true;
			return tNoStateVal;
		}

		void start() {
			mIter = mStart;
			mTrapState = false;
		}

		[[nodiscard]] uhalni nVertices() const {
			return (uhalni) mVertices.length();
		}
		
		[[nodiscard]] Range<tAlphabetType> getRange() const {
			return mAlphabetRange;
		}

	private:
		[[nodiscard]] Range<tAlphabetType> getAlphabetRange() const {
			Range<tAlphabetType> out;
			for (auto vertex : mVertices) {
				vertex.data().marked = false;
			}

			bool first = true;
			getAlphabetRangeUtil(mStart, &out, first);
			out.mEnd++;
			return out;
		}

		void getAlphabetRangeUtil(Vertex* vert, Range<tAlphabetType>* out, bool& first) const {
			vert->marked = true;
			for (auto edge : vert->edges) {
				auto const code = edge.data().transition_code;

				if (first) {
					*out = { code, code };
					first = false;
				}

				if (code < out->mBegin) {
					out->mBegin = code;
				}
				if (code > out->mEnd) {
					out->mEnd = code;
				}

				if (!edge.data().vertex->marked) {
					getAlphabetRangeUtil(edge.data().vertex, out, first);
				}
			}
		}

		void collapseEquivalentVertices() {
			// TODO
		}

		Vertex* addVertex(tStateType state) {
			auto node = mVertices.addNodeBack();
			node->data.termination_state = state;
			return &node->data;
		}

		void addTransition(Vertex* from, Vertex* to, tAlphabetType transition_symbol) {
			from->edges.pushBack({ to, transition_symbol });
		}
	};

	template <typename tAlphabetType, typename tStateType, tStateType tNoStateVal, tStateType tFailedStateVal>
	class TransitionMatrix {

		static_assert(TypeTraits<tAlphabetType>::isIntegral, "tAlphabetType must be enumerable.");

		Buffer2D<ualni> mTransitions;
		Buffer<tStateType> mStates;
		Range<tAlphabetType> mSymbolRange = { 0, 0 };
		
		ualni mIter = 0;
		ualni mIterPrev = 0;
		ualni mStart = 0;

	public:

		TransitionMatrix() = default;

		void construct(const DFA<tAlphabetType, tStateType, tNoStateVal, tFailedStateVal>& dfa) {
			mSymbolRange = dfa.getRange();
			auto range_len = ualni(mSymbolRange.mEnd - mSymbolRange.mBegin);
			auto sizeX = range_len ? range_len : 1;
			auto sizeY =  (ualni) (dfa.nVertices() + 1);

			mTransitions.reserve({ sizeX, sizeY });
			mTransitions.assign(dfa.nVertices());
			mStates.reserve(sizeY);

			ualni idx = 0;
			for (auto vertex : dfa.mVertices) {
				auto state = vertex.data().termination_state;
				mStates[idx] = state;
				idx++;
			}

			mStates[dfa.nVertices()] = tFailedStateVal;

			idx = 0;
			for (auto vertex : dfa.mVertices) {
				if (&vertex.data() == dfa.mStart) {
					mStart = mIter = mIterPrev = idx;
				}
				idx++;
			}

			ualni vertexIdx = 0;
			for (auto vertex : dfa.mVertices) {
				for (auto edge : vertex.data().edges) {
					ualni vertex2Idx = 0;
					for (auto vertex2 : dfa.mVertices) {
						if (edge.data().vertex == &vertex2.data()) break;
						vertex2Idx++;
					}
					auto const code = edge.data().transition_code;
					mTransitions.set( { (ualni) (code - mSymbolRange.mBegin), (ualni) vertexIdx }, vertex2Idx);
				}
				vertexIdx++;
			}
		}

		bool isTrapped() {
			return mStates[mIter] == tFailedStateVal;
		}

		tStateType move(tAlphabetType symbol) {
			if (symbol >= mSymbolRange.mBegin && symbol < mSymbolRange.mEnd) {
				mIter = mTransitions.get({ (ualni) (symbol - mSymbolRange.mBegin), (ualni) mIter });
			}
			else {
				mIter = mStates.size() - 1;
			}
			
			if (mIterPrev == mStart) {
				if (mStates[mIter] == tFailedStateVal) {
					reset();
					return tFailedStateVal;
				}
				else {
					mIterPrev = mIter;
					return tNoStateVal;
				}
			}
			else {
				if (mStates[mIter] == tFailedStateVal) {
					if (mStates[mIterPrev] != tNoStateVal) {
						auto out = mStates[mIterPrev];
						reset();
						return out;
					}
					else {
						reset();
						return tFailedStateVal;
					}
				}
				else {
					mIterPrev = mIter;
					return tNoStateVal;
				}
			}

			mIterPrev = mIter;
			return mStates[mIter];
		}

		void reset() {
			mIter = mStart;
			mIterPrev = mStart;
		}
	};
}