// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "OpsUtils.h"
#include "SpatialViewUtilities.h"
#include "Tests/TestDefinitions.h"

#include "SpatialView/ViewDelta.h"

#define VIEWDELTA_TEST(TestName) GDK_TEST(Core, ViewDelta, TestName)

using namespace SpatialGDK;

VIEWDELTA_TEST(Test)
{
	EntityView InputView;
	ViewDelta InputDelta;
	TArray<Worker_Op> InputOpList = { CreateAddEntityOp(2) };
	InputDelta.SetFromOpList(ConstructOpList(InputOpList), InputView);

	TArray<Worker_Op> ExpectedOpList = { CreateAddEntityOp(2) };
	EntityView ExpectedView;
	EntityViewElement Element;
	ExpectedView.Add(2, { {}, {} });
	EntityDelta EntityDelta = CreateEntityDelta(2, ADD, TArray<ComponentChange>(), TArray<AuthorityChange>());
	ViewDelta ExpectedDelta = ViewDelta({ EntityDelta });

	TestTrue("View Deltas are equal", AreEquivalent(InputDelta, ExpectedDelta));
	TestTrue("Views are equal", AreEquivalent(InputView, ExpectedView));

	return true;
}
