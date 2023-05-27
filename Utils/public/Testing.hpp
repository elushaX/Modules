
#pragma once

#include "List.hpp"

namespace tp {
  class Testing {
  public:

    struct FailedCheck {
      const char* expression = nullptr;
      const char* file = nullptr;
      ualni line = 0;
    };

    Testing() = default;
    void startTest(const char* name);
    void endTest();
    void addFailedCheck(const FailedCheck& info);
    void reportState();
    void setRootName(const char* name);
    [[nodiscard]] bool hasFailed();

  private:
    struct TestingNode {
      List<FailedCheck> mFailedChecks;
      List<TestingNode*> mSubTests;
      const char* mName = "Unnamed";
      TestingNode* mParent = nullptr;
      bool mHasFailed = false;

      void report(const char* path = nullptr) const;
      void updateState();
      ~TestingNode();
    };

    TestingNode mRootTest;
    TestingNode* mCurrent = &mRootTest;
  };

  extern Testing gTesting;
}

#define TEST_DEF(Name)\
  static void Name##FunctorBody();\
  void test##Name() { \
    tp::gTesting.startTest(#Name);\
    Name##FunctorBody();\
    tp::gTesting.endTest();\
  } \
  void Name##FunctorBody()

#define TEST_DEF_STATIC(Name)\
  static void Name##FunctorBody();\
  static void test##Name() { \
    tp::gTesting.startTest(#Name);\
    Name##FunctorBody();\
    tp::gTesting.endTest();\
  } \
  void Name##FunctorBody()

#define TEST(expr) if (!(expr)) tp::gTesting.addFailedCheck({ #expr, __FILE__, __LINE__ })
#define TEST_EQUAL(l, r) if (!((l) == (r))) tp::gTesting.addFailedCheck({ #l" == "#r, __FILE__, __LINE__ })
