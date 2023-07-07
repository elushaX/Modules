
#include "Testing.hpp"
#include "Utils.hpp"

#include <iostream>
#include <string>

using namespace tp;

Testing tp::gTesting;

void Testing::startTest(const char* name) {
	MODULE_SANITY_CHECK(gModuleUtils)
	auto newNode = new (malloc(sizeof(TestingNode))) TestingNode{ {}, {}, name, mCurrent };
	mCurrent->mSubTests.pushBack(newNode);
	mCurrent = mCurrent->mSubTests.last()->data;
}

void Testing::endTest() {
	if (mCurrent->mParent) {
		mCurrent = mCurrent->mParent;
	}
}

void Testing::addFailedCheck(const FailedCheck& info) {
	DEBUG_BREAK(0 && info.expression);
	auto lastRecord = &mCurrent->mFailedChecks.last()->data;
	if (lastRecord && lastRecord->failedCheck.file == info.file && lastRecord->failedCheck.line == info.line) {
		lastRecord->times++;
		return;
	}
	mCurrent->mFailedChecks.pushBack({ info, 1 });
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
		auto failedCheck = &check.data().failedCheck;
		auto times = check.data().times;
		printf("%s Failed - (%s) %s:%llu x%i\n", newPath.c_str(), failedCheck->expression, failedCheck->file, failedCheck->line, (halni) times);
	}

	for (const auto& child : mSubTests) {
		child->report(newPath.c_str());
	}
}

Testing::TestingNode::~TestingNode() {
	for (const auto& child : mSubTests) {
		child.data()->~TestingNode();
		free(child.data());
	}
}
