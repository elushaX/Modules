
#pragma once

#include "Utils.hpp"
#include "List.hpp"
#include "Map.hpp"

namespace tp {

	// Non-Deterministic Finite-State Automata
	template <typename tAlphabetType, typename tStateType>
	class NFA {

	public:
		struct Vertex;

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

		struct Vertex {
			List<Edge> edges;
			tStateType state{};
			bool isTermination = false;
			ualni debug_idx = 0;
			ualni flag = 0;
		};

	public:
		List<Vertex> mVertices;
		Vertex* mStart = nullptr;
		Buffer<tAlphabetType> mAllSymbols;

	public:
		NFA() = default;

		Vertex* addVertex(const tStateType& state, bool termination) {
			auto node = mVertices.newNode();
			node->data.isTermination = termination;
			node->data.state = state;
			node->data.debug_idx = mVertices.length() + 1;
			mVertices.pushBack(node);
			return &node->data;
		}

		void
		addTransition(Vertex* from, Vertex* to, Range<tAlphabetType> range, bool consumes, bool accepts_all, bool exclude) {
			Edge edge;
			edge.mVertex = to;
			edge.mConsumesSymbol = consumes;
			edge.mAcceptingRange = range;
			edge.mExclude = exclude;
			edge.mAcceptsAll = accepts_all;
			from->edges.pushBack(edge);
		}

		void setStartVertex(Vertex* start) { mStart = start; }

		[[nodiscard]] Vertex* getStartVertex() const { return mStart; }

		void setVertexState(Vertex* vertex, const tStateType& state, bool terminated) {
			vertex->state = state;
			vertex->isTermination = terminated;
		}

		[[nodiscard]] bool isValid() const {
			if (!mStart) {
				return false;
			}
			return true;
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
	template <typename tAlphabetType, typename tStateType>
	class DFA {
	public:
		struct Vertex {
			struct Edge {
				Vertex* vertex = nullptr;
				tAlphabetType transition_code = nullptr;
			};

			List<Edge> edges;
			tStateType state{};
			bool termination = false;
			bool marked = false;
		};

		List<Vertex> mVertices;
		Vertex* mStart = nullptr;
		const Vertex* mIter = nullptr;
		Buffer<tAlphabetType> mAlphabetRange;
		bool mTrapState = false;

		typedef typename NFA<tAlphabetType, tStateType>::Vertex NState;

	public:
		explicit DFA(NFA<tAlphabetType, tStateType>& nfa) {
			if (!nfa.isValid()) {
				return;
			}

			mAlphabetRange = nfa.mAllSymbols;

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

				ualni debug_idx = 0;
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
					auto idx = dStates.presents({ &reachableNStates });
					if (idx) {
						targetDState = dStates.getSlotVal(idx);
					}

					if (!targetDState) {
						// register new DFA state
						targetDState = new DState();

						targetDState->debug_idx = dStates.size();

						targetDState->nStates = reachableNStates;

						// append to working stack
						working_set.pushBack(targetDState);
						dStates.put({ &targetDState->nStates }, targetDState);
					}

					// add transition to DFA state
					currentDState->data->transitions.pushBack({ targetDState, symbol });
				}

				working_set.popFront();
				currentDState = working_set.first();
			}

			// create own vertices
			for (auto node : dStates) {
				const tStateType* state = nullptr;
				ualni numFound = 0;
				for (auto iter : node->val->nStates) {
					if (iter->isTermination) {
						state = &iter->state;
						break;
					}
				}
				if (numFound != 1) {
					printf("Error constructing dfa - invalid number of termination states in final node.");
					exit(1);
				}
				node->val->dVertex = addVertex(*state);
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
		}

		/*
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
		*/

		[[nodiscard]] uhalni nVertices() const { return (uhalni) mVertices.length(); }

		[[nodiscard]] const Buffer<tAlphabetType>& getRange() const { return mAlphabetRange; }

	public:
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

		Vertex* addVertex(const tStateType& state) {
			auto node = mVertices.addNodeBack();
			node->data.state = state;
			return &node->data;
		}

		void addTransition(Vertex* from, Vertex* to, tAlphabetType transition_symbol) {
			from->edges.pushBack({ to, transition_symbol });
		}
	};
}