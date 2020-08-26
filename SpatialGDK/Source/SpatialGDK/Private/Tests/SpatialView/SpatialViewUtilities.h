// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "EntityComponentTestUtils.h"
#include "SpatialView/ViewDelta.h"
#include "SpatialView/WorkerView.h"

namespace SpatialGDK
{
enum ENTITY_STATUS
{
	ADD,
	REMOVE,
	UPDATE
};

inline EntityDelta CreateEntityDelta(long EntityId, ENTITY_STATUS Status, TArray<ComponentChange> ComponentChanges,
									 TArray<AuthorityChange> AuthorityChanges)
{
	EntityDelta Delta;
	Delta.EntityId = EntityId;
	Delta.bAdded = Status == ADD;
	Delta.bRemoved = Status == REMOVE;

	TArray<ComponentChange> ComponentsAdded;
	TArray<ComponentChange> ComponentsRemoved;
	TArray<ComponentChange> ComponentUpdates;
	TArray<ComponentChange> ComponentsRefreshed;
	for (const ComponentChange& Change : ComponentChanges)
	{
		switch (Change.Type)
		{
		case ComponentChange::ADD:
			ComponentsAdded.Push(Change);
		case ComponentChange::REMOVE:
			ComponentsRemoved.Push(Change);
		case ComponentChange::UPDATE:
			ComponentUpdates.Push(Change);
		case ComponentChange::COMPLETE_UPDATE:
			ComponentsRefreshed.Push(Change);
		default:
			break;
		}
	}
	Delta.ComponentsAdded = { ComponentsAdded.GetData(), ComponentsAdded.Num() };
	Delta.ComponentsRemoved = { ComponentsRemoved.GetData(), ComponentsRemoved.Num() };
	Delta.ComponentUpdates = { ComponentUpdates.GetData(), ComponentUpdates.Num() };
	Delta.ComponentsRefreshed = { ComponentsRefreshed.GetData(), ComponentsRefreshed.Num() };

	TArray<AuthorityChange> AuthorityLost;
	TArray<AuthorityChange> AuthorityGained;
	TArray<AuthorityChange> AuthorityLostTemporarily;
	for (const AuthorityChange& Change : AuthorityChanges)
	{
		switch (Change.Type)
		{
		case AuthorityChange::AUTHORITY_LOST:
			AuthorityLost.Push(Change);
		case AuthorityChange::AUTHORITY_GAINED:
			AuthorityGained.Push(Change);
		case AuthorityChange::AUTHORITY_LOST_TEMPORARILY:
			AuthorityLostTemporarily.Push(Change);
		default:
			break;
		}
	}
	Delta.AuthorityLost = { AuthorityLost.GetData(), AuthorityLost.Num() };
	Delta.AuthorityGained = { AuthorityGained.GetData(), AuthorityGained.Num() };
	Delta.AuthorityLostTemporarily = { AuthorityLostTemporarily.GetData(), AuthorityLostTemporarily.Num() };

	return Delta;
}

inline bool AreEquivalent(EntityView& Lhs, EntityView& Rhs)
{
	TArray<Worker_EntityId_Key> LhsKeys;
	TArray<Worker_EntityId_Key> RhsKeys;
	Lhs.GetKeys(LhsKeys);
	Rhs.GetKeys(RhsKeys);
	if (!AreEquivalent(LhsKeys, RhsKeys, CompareWorkerEntityIdKey))
	{
		return false;
	}

	for (const auto& Pair : Lhs)
	{
		long EntityId = Pair.Key;
		const EntityViewElement* LhsElement = &Pair.Value;
		const EntityViewElement* RhsElement = &Rhs[EntityId];
		if (!AreEquivalent(LhsElement->Components, RhsElement->Components))
		{
			return false;
		}

		if (!AreEquivalent(LhsElement->Authority, RhsElement->Authority, CompareWorkerComponentId))
		{
			return false;
		}
	}

	return false;
}

inline bool AreEquivalent(ViewDelta& Lhs, ViewDelta& Rhs)
{
	// This does not check for events or commands atm
	TArray<EntityDelta> LhsEntityDeltas = Lhs.GetEntityDeltas();
	TArray<EntityDelta> RhsEntityDeltas = Rhs.GetEntityDeltas();
	if (LhsEntityDeltas.Num() != RhsEntityDeltas.Num())
	{
		return false;
	}

	for (int i = 0; i < LhsEntityDeltas.Num(); i++)
	{
		EntityDelta LhsEntityDelta = LhsEntityDeltas[i];
		EntityDelta RhsEntityDelta = RhsEntityDeltas[i];
		if (LhsEntityDelta.EntityId != RhsEntityDelta.EntityId)
		{
			return false;
		}

		if (LhsEntityDelta.bAdded != RhsEntityDelta.bAdded)
		{
			return false;
		}

		if (LhsEntityDelta.bRemoved != RhsEntityDelta.bRemoved)
		{
			return false;
		}

		if (!AreEquivalent(LhsEntityDelta.AuthorityGained, RhsEntityDelta.AuthorityGained, CompareAuthorityChanges))
		{
			return false;
		}

		if (!AreEquivalent(LhsEntityDelta.AuthorityLost, RhsEntityDelta.AuthorityLost, CompareAuthorityChanges))
		{
			return false;
		}

		if (!AreEquivalent(LhsEntityDelta.AuthorityLostTemporarily, RhsEntityDelta.AuthorityLostTemporarily, CompareAuthorityChanges))
		{
			return false;
		}

		if (!AreEquivalent(LhsEntityDelta.ComponentsAdded, RhsEntityDelta.ComponentsAdded, CompareComponentChanges))
		{
			return false;
		}

		if (!AreEquivalent(LhsEntityDelta.ComponentsRemoved, RhsEntityDelta.ComponentsRemoved, CompareComponentChanges))
		{
			return false;
		}

		if (!AreEquivalent(LhsEntityDelta.ComponentsRefreshed, RhsEntityDelta.ComponentsRefreshed, CompareComponentChanges))
		{
			return false;
		}

		if (!AreEquivalent(LhsEntityDelta.ComponentUpdates, RhsEntityDelta.ComponentUpdates, CompareComponentChanges))
		{
			return false;
		}
	}

	return true;
}
} // namespace SpatialGDK
