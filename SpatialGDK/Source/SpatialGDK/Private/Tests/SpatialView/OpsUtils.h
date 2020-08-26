// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "EntityComponentTestUtils.h"
#include "SpatialView/WorkerView.h"

namespace SpatialGDK
{
inline TArray<OpList> ConstructOpList(TArray<Worker_Op>& Ops)
{
	TArray<OpList> OpLists;
	OpLists.Push({ Ops.GetData(), static_cast<uint32_t>(Ops.Num()), nullptr });
	return OpLists;
}

inline Worker_Op CreateAddEntityOp(int EntityId)
{
	Worker_Op Op{};
	Op.op_type = WORKER_OP_TYPE_ADD_ENTITY;
	Op.op.add_entity = Worker_AddEntityOp();
	Op.op.add_entity.entity_id = EntityId;
	return Op;
}

inline Worker_Op CreateRemoveEntityOp(int EntityId)
{
	Worker_Op Op{};
	Op.op_type = WORKER_OP_TYPE_REMOVE_ENTITY;
	Op.op.remove_entity = Worker_RemoveEntityOp();
	Op.op.remove_entity.entity_id = EntityId;
	return Op;
}

inline Worker_Op CreateAuthorityChangeOp(int EntityId, int ComponentId, Worker_Authority Authority)
{
	Worker_Op Op{};
	Op.op_type = WORKER_OP_TYPE_AUTHORITY_CHANGE;
	Op.op.authority_change = Worker_AuthorityChangeOp();
	Op.op.authority_change.entity_id = EntityId;
	Op.op.authority_change.component_id = ComponentId;
	Op.op.authority_change.authority = Authority;
	return Op;
}

inline Worker_Op CreateAddComponentOp(int EntityId, int ComponentId)
{
	Worker_Op Op{};
	Op.op_type = WORKER_OP_TYPE_ADD_COMPONENT;
	Op.op.add_component = Worker_AddComponentOp();
	ComponentData Data = CreateTestComponentData(ComponentId, 20);
	Op.op.add_component.entity_id = EntityId;
	Op.op.add_component.data.component_id = Data.GetComponentId();
	Op.op.add_component.data.schema_type = Data.GetUnderlying();
	return Op;
}

inline Worker_Op CreateRemoveComponentOp(int EntityId, int ComponentId)
{
	Worker_Op Op{};
	Op.op_type = WORKER_OP_TYPE_REMOVE_COMPONENT;
	Op.op.remove_component = Worker_RemoveComponentOp();
	Op.op.remove_component.entity_id = EntityId;
	Op.op.remove_component.component_id = ComponentId;
	return Op;
}
} // namespace SpatialGDK
