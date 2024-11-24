
#include "Archiver.hpp"
#include <cstdio>
#include <cstdlib>

using namespace tp;

bool sFailed = false;

struct Undef {
  char character1 = 'a';
  bool boolean = true;
  ualni integer = 321;
  char character2 = 'a';

  void change() {
    character1++;
    character2--;
    integer++;
    boolean = !boolean;
  }

  [[nodiscard]] bool operator!=(const Undef& in) const {
    if (character1 != in.character1) {
      return true;
    }
    if (character2 != in.character2) {
      return true;
    }
    if (integer != in.integer) {
      return true;
    }
    if (boolean != in.boolean) {
      return true;
    }
    return false;
  }
};

struct SimpleArchive {
  char character = 'a';
  ualni integer = 123;
  bool boolean = true;
  Undef undef;

  template <typename tArchiver>
  void archive(tArchiver& ar) {
    ar % character;
    ar % integer;
    ar % boolean;
    ar % undef;
  }

  void change() {
    character++;
    integer++;
    boolean = !boolean;
    undef.change();
  }

  [[nodiscard]] bool operator!=(const SimpleArchive& in) const {
    if (character != in.character) {
      return true;
    }
    if (integer != in.integer) {
      return true;
    }
    if (boolean != in.boolean) {
      return true;
    }
    if (undef != in.undef) {
      return true;
    }
    return false;
  }
};

struct Simple {
  char character = 'a';
  ualni integer = 123;
  bool boolean = true;
  Undef undef;

  template <typename tArchiver>
  void archiveRead(tArchiver& ar) {
    ar >> character;
    ar >> integer;
    ar >> boolean;
    ar >> undef;
  }

  template <typename tArchiver>
  void archiveWrite(tArchiver& ar) const {
    ar << character;
    ar << integer;
    ar << boolean;
    ar << undef;
  }

  void change() {
    character++;
    integer++;
    boolean = !boolean;
    undef.change();
  }

  [[nodiscard]] bool operator!=(const Simple& in) const {
    if (character != in.character) {
      return true;
    }
    if (integer != in.integer) {
      return true;
    }
    if (boolean != in.boolean) {
      return true;
    }
    if (undef != in.undef) {
      return true;
    }
    return false;
  }
};

template <typename tType>
struct Set {
  tType buff[10];
  ualni len = 5;

  template <typename tArchiver>
  void archiveRead(tArchiver& ar) {
    ar >> len;
    for (auto i = 0; i < len; i++) {
      ar >> buff[i];
    }
  }

  template <typename tArchiver>
  void archiveWrite(tArchiver& ar) const {
    ar << len;
    for (auto i = 0; i < len; i++) {
      ar << buff[i];
    }
  }

  void change() {
    len = 2;
    for (auto i = 0; i < len; i++) {
      buff[i].change();
    }
  }

  [[nodiscard]] bool operator!=(const Set& in) const {
    for (auto i = 0; i < len; i++) {
      if (buff[i] != in.buff[i]) {
        return true;
      }
    }

    if (len != in.len) {
      return true;
    }
    return false;
  }
};

template <typename tType>
struct ComplexCart {
  Undef undef;
  Set<tType> set;

  template <typename tArchiver>
  void archive(tArchiver& ar) {
    ar % set;
    ar % undef;
  }

  void change() {
    undef.change();
    set.change();
  }

  [[nodiscard]] bool operator!=(const ComplexCart& in) const { return undef != in.undef || set != in.set; }
};

template <typename tType>
void test() {
  constexpr auto size = 1024;

  const tType val;
  tType res;

  res.change();

  ArchiverExample<size, false> write;
  ArchiverExample<size, true> read;

  write % val;

  for (auto i = 0; i < size; i++) {
    read.mBuff[i] = write.mBuff[i];
  }

  read % res;

  if (val != res) {
    printf("Test %s Failed\n", typeid(tType).name());
    sFailed = true;
  }
}

template <typename T, typename A, typename = void>
struct HasArchive : FalseType {};

template <typename T, typename A>
struct HasArchive<T, A, VoidType<decltype(DeclareValue<T>()().archive(DeclareValue<A&>()()))>> : TrueType {};

void testArchiver() {
  printf("has archive method - %i\n", HasArchive<SimpleArchive, ArchiverExample<10, false>>::value);
  printf("has archive method - %i\n", ArchiverExample<10, false>::HasFunc::Archive<SimpleArchive>::value);
  printf("has archive method - %i\n", ArchiverExample<10, false>::HasFunc::Archive<Simple>::value);

  test<SimpleArchive>();
  test<Simple>();
  test<Set<Simple>>();
  test<ComplexCart<Simple>>();

  if (sFailed) {
    exit(1);
  }
}
