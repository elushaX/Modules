
#pragma once

/*
 * This file contains acceptors for each language
 * Acceptor is managing an automation machine that can recognize whether this given sentence is in the language.
 * Above that acceptors are responsible for defining those automations
 * Some acceptors can supply additional information such as Abstract-Syntax-Tree or Sentence id
 * */

#include "LanguageModule.hpp"

namespace tp {

  template <typename tAlphabet>
  class SymbolSupply {
  public:
    typedef ualni Cursor;

  public:
    SymbolSupply() = default;
    virtual ~SymbolSupply() = 0;

  public:
    virtual Cursor getCursor() = 0;
    virtual void setCursor(Cursor) = 0;

    virtual void getPrevCursor() = 0;
    virtual ualni getSymbolLength() = 0;

    virtual const tAlphabet& getNextSymbol() = 0;

  private:
    Cursor mCursor = 0;
  };

  template <typename tAlphabet, typename tSentenceId>
  class RegularAcceptor {
    typedef SymbolSupply<tAlphabet> Sentence;

  public:
    RegularAcceptor() = default;
    void build(const RegularGrammar<tSentenceId>& grammar);
    tSentenceId recognize(Sentence& sentence);
  };

  // Subset of CFG
  template <typename tAlphabet>
  class LL {
    typedef SymbolSupply<tAlphabet> Sentence;

  public:
    LL() = default;
    void build(const ContextFreeGrammar& grammar);
    void recognize(Sentence& sentence);
  };

  // Subset of CFG much larger than LL
  template <typename tAlphabet>
  class CLR {
    typedef SymbolSupply<tAlphabet> Sentence;

  public:
    CLR() = default;
    void build(const ContextFreeGrammar& grammar);
    void recognize(Sentence& sentence);
  };

  // Subset of CFG CLR
  template <typename tAlphabet>
  class LALR {
    typedef SymbolSupply<tAlphabet> Sentence;

  public:
    LALR() = default;
    void build(const ContextFreeGrammar& grammar);
    void recognize(Sentence& sentence);
  };
}
