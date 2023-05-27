
#include "Testing.hpp"
#include "Utils.hpp"

#include <iostream>
#include <string>

using namespace tp;

Testing tp::gTesting;

void Testing::startTest(const char* name) {
  MODULE_SANITY_CHECK(gModuleUtils)
  mCurrent->mSubTests.pushBack(new TestingNode{ {}, {}, name, mCurrent });
  mCurrent = mCurrent->mSubTests.last()->data;
}

void Testing::endTest() {
  if (mCurrent->mParent) {
    mCurrent = mCurrent->mParent;
  }
}

void Testing::addFailedCheck(const FailedCheck& info) {
  mCurrent->mFailedChecks.pushBack(info);
}

void Testing::reportState() {
  mRootTest.updateState();

  printf("\n");
  mRootTest.report();
}

bool Testing::hasFailed() {
  mRootTest.updateState();
  return mRootTest.mHasFailed;
}

void Testing::setRootName(const char* name) {
  mRootTest.mName = name;
}

void Testing::TestingNode::updateState() {
  for (auto child : mSubTests) {
    child->updateState();
    mHasFailed = child->mHasFailed;
    if (mHasFailed) return;
  }
  mHasFailed = mFailedChecks.length();
}

void Testing::TestingNode::report(const char* path) const {
  if (!mHasFailed) {
    return;
  }

  auto newPath = path ? std::string(path) + "/" + mName : std::string(mName);

  for (auto check : mFailedChecks) {
    printf("%s Failed - (%s) %s:%llu\n", newPath.c_str(), check.data().expression, check.data().file, check.data().line);
  }

  for (const auto& child : mSubTests) {
    child->report(newPath.c_str());
  }
}

Testing::TestingNode::~TestingNode() {
  for (const auto& child : mSubTests) {
    delete child.data();
  }
}
