// The MIT License (MIT)
//
// 	Copyright (c) 2015 Sergey Makeev, Vadim Slyusarev
//
// 	Permission is hereby granted, free of charge, to any person obtaining a copy
// 	of this software and associated documentation files (the "Software"), to deal
// 	in the Software without restriction, including without limitation the rights
// 	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// 	copies of the Software, and to permit persons to whom the Software is
// 	furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
// 	all copies or substantial portions of the Software.
//
// 	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// 	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// 	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// 	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// 	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// 	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// 	THE SOFTWARE.

#include "Tests.h"
#include <UnitTest++.h>
#include <MTScheduler.h>


SUITE(ScopesTests)
{


struct AssetStackEntry : public MT::ScopeStackEntry
{
	const char* assetName;

	AssetStackEntry(int32 parentIndex, int32 descIndex, const char* _assetName)
		: MT::ScopeStackEntry(parentIndex, descIndex)
		, assetName(_assetName)
	{
	}
};

//current thread scopes stack (unique per thread)
static mt_thread_local MT::StrongScopeStack<AssetStackEntry, 128>* threadScopesStack = nullptr;

//global scope descriptors storage
static MT::PersistentScopeDescriptorStorage<MT::ScopeDesc, 128>* globalScopesStorage = nullptr;

const char* assetName1 = "some_resource_name.asset";
const char* assetName2 = "some_another_name.asset";

void MacroSyntaxCheckerFunction()
{
	int32 descId = MT::invalidStackId;

	//declare scope descriptor
	DECLARE_SCOPE_DESCRIPTOR("test", globalScopesStorage, descId);

	//push to stack
	SCOPE_STACK_PUSH1(descId, assetName1, threadScopesStack);

	AssetStackEntry* stackTop = SCOPE_STACK_TOP(threadScopesStack);
	CHECK(stackTop != nullptr);
	int32 parentStackId = stackTop->GetParentId();
	CHECK(parentStackId == MT::invalidStackId);

	{
		int32 innerDescId = MT::invalidStackId;

		DECLARE_SCOPE_DESCRIPTOR("test2", globalScopesStorage, innerDescId);
		SCOPE_STACK_PUSH1(innerDescId, assetName2, threadScopesStack);

		AssetStackEntry* assetStackEntry = SCOPE_STACK_TOP(threadScopesStack);

		CHECK(assetStackEntry->assetName == assetName2);

		assetStackEntry = SCOPE_STACK_GET_PARENT(assetStackEntry, threadScopesStack);
		CHECK(assetStackEntry->assetName == assetName1);

		SCOPE_STACK_POP(threadScopesStack);
	}

	//pop from stack
	SCOPE_STACK_POP(threadScopesStack);
}


TEST(MacroSytnaxCheck)
{
	globalScopesStorage = new MT::PersistentScopeDescriptorStorage<MT::ScopeDesc, 128>();
	threadScopesStack = new MT::StrongScopeStack<AssetStackEntry, 128>();

	for(int i = 0; i < 16; i++)
	{
		MacroSyntaxCheckerFunction();
	}

	delete globalScopesStorage;
	globalScopesStorage = nullptr;

	delete threadScopesStack;
	threadScopesStack = nullptr;
}


TEST(WeakStackTest)
{
	MT::WeakScopeStack<MT::ScopeStackEntry, 64> weakStack;
	
	//new stack must be empty
	int parentId = weakStack.Top();
	CHECK(parentId == MT::invalidStackId);

	MT::ScopeStackEntry* pInstance = weakStack.Push(parentId, MT::invalidStorageId);
	CHECK(pInstance != nullptr);

	//check instance data
	CHECK(parentId == pInstance->GetParentId());
	CHECK(MT::invalidStorageId == pInstance->GetDescriptionId());

	// check updated stack top
	int stackTopId = weakStack.Top();
	CHECK(stackTopId != MT::invalidStackId);
	CHECK(weakStack.Get(stackTopId) == pInstance);

	int32 testStorageId = 3;

	MT::ScopeStackEntry* pInstance2 = weakStack.Push(stackTopId, testStorageId);

	CHECK(pInstance2 != pInstance);
	CHECK(stackTopId == pInstance2->GetParentId());
	CHECK(testStorageId == pInstance2->GetDescriptionId());

	int stackTopId2 = weakStack.Top();
	CHECK(stackTopId2 != MT::invalidStackId);
	CHECK(stackTopId2 != stackTopId);
	CHECK(weakStack.Get(stackTopId2) == pInstance2);

	weakStack.Pop();

	int _stackTopId = weakStack.Top();
	CHECK(stackTopId == _stackTopId);

	weakStack.Pop();
	CHECK(weakStack.Top() == MT::invalidStackId);
}

TEST(StrongStackTest)
{
	MT::StrongScopeStack<MT::ScopeStackEntry, 64> strongStack;

	//new stack must be empty
	int parentId = strongStack.Top();
	CHECK(parentId == MT::invalidStackId);

	MT::ScopeStackEntry* pInstance = strongStack.Push(parentId, MT::invalidStorageId);
	CHECK(pInstance != nullptr);

	//check instance data
	CHECK(parentId == pInstance->GetParentId());
	CHECK(MT::invalidStorageId == pInstance->GetDescriptionId());

	// check updated stack top
	int stackTopId = strongStack.Top();
	CHECK(stackTopId != MT::invalidStackId);
	CHECK(strongStack.Get(stackTopId) == pInstance);

	int32 testStorageId = 3;

	MT::ScopeStackEntry* pInstance2 = strongStack.Push(stackTopId, testStorageId);

	CHECK(pInstance2 != pInstance);
	CHECK(stackTopId == pInstance2->GetParentId());
	CHECK(testStorageId == pInstance2->GetDescriptionId());

	int stackTopId2 = strongStack.Top();
	CHECK(stackTopId2 != MT::invalidStackId);
	CHECK(stackTopId2 != stackTopId);
	CHECK(strongStack.Get(stackTopId2) == pInstance2);

	strongStack.Pop();

	int _stackTopId = strongStack.Top();
	CHECK(stackTopId == _stackTopId);

	strongStack.Pop();
	CHECK(strongStack.Top() == MT::invalidStackId);

	// strong stack is means that we can still use stack pointers (and Id's) after pop
	CHECK(strongStack.Get(stackTopId) == pInstance);
	CHECK(strongStack.Get(stackTopId2) == pInstance2);

	CHECK(stackTopId == pInstance2->GetParentId());
	CHECK(testStorageId == pInstance2->GetDescriptionId());

	CHECK(parentId == pInstance->GetParentId());
	CHECK(MT::invalidStorageId == pInstance->GetDescriptionId());


	MT::ScopeStackEntry* pInstance3 = strongStack.Push(parentId, MT::invalidStorageId);
	CHECK(pInstance3 != nullptr);

	// strong stack can't reuse memory
	// because it ensures that the memory is always accessible, even after popping from stack
	CHECK(pInstance3 != pInstance);


}

TEST(PersistentStorageTest)
{
	MT::PersistentScopeDescriptorStorage<MT::ScopeDesc, 128> persistentStorage;

	const char* srcFile = __FILE__;
	int32 srcLine = __LINE__;
	const char* scopeName1 = "TestScope1";
	const char* scopeName2 = "TestScope2";

	int32 id1 = persistentStorage.Alloc(srcFile, srcLine, scopeName1);
	CHECK(id1 != MT::invalidStorageId);

	int32 id2 = persistentStorage.Alloc(srcFile, srcLine, scopeName2);
	CHECK(id2 != MT::invalidStorageId);
	
	MT::ScopeDesc* desc1 = persistentStorage.Get(id1);
	CHECK(desc1 != nullptr);

	CHECK(desc1->GetSourceFile() == srcFile);
	CHECK(desc1->GetSourceLine() == srcLine);
	CHECK(desc1->GetName() == scopeName1);

	MT::ScopeDesc* desc2 = persistentStorage.Get(id2);
	CHECK(desc2 != nullptr);

	CHECK(desc2->GetSourceFile() == srcFile);
	CHECK(desc2->GetSourceLine() == srcLine);
	CHECK(desc2->GetName() == scopeName2);
}



TEST(ComplexStressTest)
{
	//TODO
}

}
