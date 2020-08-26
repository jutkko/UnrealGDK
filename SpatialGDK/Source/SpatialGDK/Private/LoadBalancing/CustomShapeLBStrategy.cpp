// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "LoadBalancing/CustomShapeLBStrategy.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "Utils/SpatialActorUtils.h"

#include "Templates/Tuple.h"

DEFINE_LOG_CATEGORY(LogCustomShapeLBStrategy);

UCustomShapeLBStrategy::UCustomShapeLBStrategy()
	: Super()
	, Rows(1)
	, Cols(1)
	, WorldWidth(1000000.f)
	, WorldHeight(1000000.f)
	, InterestBorder(0.f)
	, LocalCellId(0)
	, bIsStrategyUsedOnLocalWorker(false)
{
}

// Always ignore the columns in the first row, while keeping the others the same as grid
void UCustomShapeLBStrategy::Init()
{
	Super::Init();
	UE_LOG(LogCustomShapeLBStrategy, Log, TEXT("CustomShapeLBStrategy initialized with Rows = %d and Cols = %d."), 2, 2);

	const float WorldWidthMin = -(WorldWidth / 2.f);
	const float WorldHeightMin = -(WorldHeight / 2.f);

	// const float ColumnWidth = WorldWidth / Cols;
	// const float RowHeight = WorldHeight / Rows;

	const float ColumnWidth = WorldWidth / 2;
	const float RowHeight = WorldHeight / 2;

	// We would like the inspector's representation of the load balancing strategy to match our intuition.
	// +x is forward, so rows are perpendicular to the x-axis and columns are perpendicular to the y-axis.
	float XMin = WorldHeightMin;
	float YMin = WorldWidthMin;
	float XMax, YMax;

	// Create a combination of 1, 2, 3; 4; LB shape
	uint32 areaCount = 1;
	// Initialise 2 WorkerCellsSets
	TArray<FBox2D> WorkerCells1;
	TArray<FBox2D> WorkerCells2;

	for (uint32 Col = 0; Col < 2; ++Col)
	{
		YMax = YMin + ColumnWidth;

		for (uint32 Row = 0; Row < 2; ++Row)
		{
			XMax = XMin + RowHeight;

			FVector2D Min(XMin, YMin);
			FVector2D Max(XMax, YMax);
			FBox2D Cell(Min, Max);
			if (areaCount != 4)
			{
				WorkerCells1.Add(Cell);
			}
			else {
				WorkerCells2.Add(Cell);
			}

			XMin = XMax;
			areaCount++;
		}

		XMin = WorldHeightMin;
		YMin = YMax;
	}

	WorkerCellsSet.Add(WorkerCells1);
	WorkerCellsSet.Add(WorkerCells2);
}

void UCustomShapeLBStrategy::SetLocalVirtualWorkerId(VirtualWorkerId InLocalVirtualWorkerId)
{
	if (!VirtualWorkerIds.Contains(InLocalVirtualWorkerId))
	{
		// This worker is simulating a layer which is not part of the grid.
		LocalCellId = WorkerCellsSet.Num();
		bIsStrategyUsedOnLocalWorker = false;
	}
	else
	{
		LocalCellId = VirtualWorkerIds.IndexOfByKey(InLocalVirtualWorkerId);
		bIsStrategyUsedOnLocalWorker = true;
	}
	LocalVirtualWorkerId = InLocalVirtualWorkerId;
}

TSet<VirtualWorkerId> UCustomShapeLBStrategy::GetVirtualWorkerIds() const
{
	return TSet<VirtualWorkerId>(VirtualWorkerIds);
}

bool UCustomShapeLBStrategy::ShouldHaveAuthority(const AActor& Actor) const
{
	if (!IsReady())
	{
		UE_LOG(LogCustomShapeLBStrategy, Warning, TEXT("CustomShapeLBStrategy not ready to relinquish authority for Actor %s."), *AActor::GetDebugName(&Actor));
		return false;
	}

	if (!bIsStrategyUsedOnLocalWorker)
	{
		return false;
	}

	const FVector2D Actor2DLocation = FVector2D(SpatialGDK::GetActorSpatialPosition(&Actor));
	for (int i = 0; i < WorkerCellsSet[LocalCellId].Num(); ++i)
	{
		if (IsInside(WorkerCellsSet[LocalCellId][i], Actor2DLocation))
		{
			return true;
		}
	}
	return false;
}

VirtualWorkerId UCustomShapeLBStrategy::WhoShouldHaveAuthority(const AActor& Actor) const
{
	if (!IsReady())
	{
		UE_LOG(LogCustomShapeLBStrategy, Warning, TEXT("CustomShapeLBStrategy not ready to decide on authority for Actor %s."), *AActor::GetDebugName(&Actor));
		return SpatialConstants::INVALID_VIRTUAL_WORKER_ID;
	}

	const FVector2D Actor2DLocation = FVector2D(SpatialGDK::GetActorSpatialPosition(&Actor));

	check(VirtualWorkerIds.Num() == WorkerCellsSet.Num());
	for (int i = 0; i < WorkerCellsSet.Num(); ++i)
	{
		for (int j = 0; j < WorkerCellsSet[i].Num(); ++j)
		{
			if (IsInside(WorkerCellsSet[i][j], Actor2DLocation))
			{
				UE_LOG(LogCustomShapeLBStrategy, Log, TEXT("Actor: %s, grid %d, worker %d for position %s"), *AActor::GetDebugName(&Actor), i, VirtualWorkerIds[i], *Actor2DLocation.ToString());
				return VirtualWorkerIds[i];
			}
		}
	}

	UE_LOG(LogCustomShapeLBStrategy, Error, TEXT("CustomShapeLBStrategy couldn't determine virtual worker for Actor %s at position %s"), *AActor::GetDebugName(&Actor), *Actor2DLocation.ToString());
	return SpatialConstants::INVALID_VIRTUAL_WORKER_ID;
}

SpatialGDK::QueryConstraint UCustomShapeLBStrategy::GetWorkerInterestQueryConstraint() const
{
	// For a grid-based strategy, the interest area is the cell that the worker is authoritative over plus some border region.
	check(IsReady());
	check(bIsStrategyUsedOnLocalWorker);

	SpatialGDK::QueryConstraint Constraint;
	for (int i = 0; i < WorkerCellsSet[LocalCellId].Num(); ++i)
	{
		const FBox2D Interest2D = WorkerCellsSet[LocalCellId][i].ExpandBy(InterestBorder);

		const FVector2D Center2D = Interest2D.GetCenter();
		const FVector Center3D{ Center2D.X, Center2D.Y, 0.0f };

		const FVector2D EdgeLengths2D = Interest2D.GetSize();
		check(EdgeLengths2D.X > 0.0f && EdgeLengths2D.Y > 0.0f);
		const FVector EdgeLengths3D{ EdgeLengths2D.X, EdgeLengths2D.Y, FLT_MAX };
		SpatialGDK::QueryConstraint BoxConstraint;

		BoxConstraint.BoxConstraint = SpatialGDK::BoxConstraint{ SpatialGDK::Coordinates::FromFVector(Center3D), SpatialGDK::EdgeLength::FromFVector(EdgeLengths3D) };
		Constraint.OrConstraint.Add(BoxConstraint);
	}

	return Constraint;
}

FVector UCustomShapeLBStrategy::GetWorkerEntityPosition() const
{
	check(IsReady());
	check(bIsStrategyUsedOnLocalWorker);
	const FVector2D Centre = WorkerCellsSet[LocalCellId][0].GetCenter();
	return FVector{ Centre.X, Centre.Y, 0.f };
}

uint32 UCustomShapeLBStrategy::GetMinimumRequiredWorkers() const
{
	return Rows * Cols;
}

void UCustomShapeLBStrategy::SetVirtualWorkerIds(const VirtualWorkerId& FirstVirtualWorkerId, const VirtualWorkerId& LastVirtualWorkerId)
{
	UE_LOG(LogCustomShapeLBStrategy, Log, TEXT("Setting VirtualWorkerIds %d to %d"), FirstVirtualWorkerId, LastVirtualWorkerId);
	for (VirtualWorkerId CurrentVirtualWorkerId = FirstVirtualWorkerId; CurrentVirtualWorkerId <= LastVirtualWorkerId; CurrentVirtualWorkerId++)
	{
		VirtualWorkerIds.Add(CurrentVirtualWorkerId);
	}
}

bool UCustomShapeLBStrategy::IsInside(const FBox2D& Box, const FVector2D& Location)
{
	return Location.X >= Box.Min.X && Location.Y >= Box.Min.Y
		&& Location.X < Box.Max.X && Location.Y < Box.Max.Y;
}
