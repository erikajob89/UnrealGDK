// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "StandardLibrary.h"

namespace improbable
{

struct ComponentInterest
{
	struct SphereConstraint
	{
		Coordinates Center;
		double Radius;
	};

	struct CylinderConstraint
	{
		Coordinates Center;
		double Radius;
	};

	struct BoxConstraint
	{
		Coordinates Center;
		EdgeLength EdgeLength;
	};

	struct RelativeSphereConstraint
	{
		double Radius;
	};

	struct RelativeCylinderConstraint
	{
		double Radius;
	};

	struct RelativeBoxConstraint
	{
		EdgeLength EdgeLength;
	};

	struct QueryConstraint
	{
		TSchemaOption<SphereConstraint> SphereConstraint;
		TSchemaOption<CylinderConstraint> CylinderConstraint;
		TSchemaOption<BoxConstraint> BoxConstraint;
		TSchemaOption<RelativeSphereConstraint> RelativeSphereConstraint;
		TSchemaOption<RelativeCylinderConstraint> RelativeCylinderConstraint;
		TSchemaOption<RelativeBoxConstraint> RelativeBoxConstraint;
		TSchemaOption<int64> EntityIdConstraint;
		TSchemaOption<uint32> ComponentConstraint;
		TArray<QueryConstraint> AndConstraint;
		TArray<QueryConstraint> OrConstraint;
	};

	struct Query
	{
		QueryConstraint Constraint;

		// Either full_snapshot_result or a list of result_component_id should be provided. Providing both is invalid.
		TSchemaOption<bool> FullSnapshotResult; // Whether all components should be included or none.
		TArray<uint32> ResultComponentId; // Which components should be included.

		// Used for frequency-based rate limiting. Represents the maximum frequency of updates for this
		// particular query. An empty option represents no rate-limiting (ie. updates are received
		// as soon as possible). Frequency is measured in Hz.
		//
		// If set, the time between consecutive updates will be at least 1/frequency. This is determined
		// at the time that updates are sent from the Runtime and may not necessarily correspond to the
		// time updates are received by the worker.
		//
		// If after an update has been sent, multiple updates are applied to a component, they will be
		// merged and sent as a single update after 1/frequency of the last sent update. When components
		// with events are merged, the resultant component will contain a concatenation of all the
		// events.
		//
		// If multiple queries match the same Entity-Component then the highest of all frequencies is
		// used.
		TSchemaOption<float> Frequency;
	};

	TArray<Query> Queries;
};

inline void AddQueryConstraintToSchema(Schema_Object* Object, Schema_FieldId Id, const ComponentInterest::QueryConstraint& Constraint)
{
	Schema_Object* QueryConstraintObject = Schema_AddObject(Object, Id);

	//option<SphereConstraint> sphere_constraint = 1;
	if (Constraint.SphereConstraint.IsSet())
	{
		Schema_Object* SphereConstraintObject = Schema_AddObject(QueryConstraintObject, 1);

		AddCoordinateToSchema(SphereConstraintObject, 1, Constraint.SphereConstraint->Center);
		Schema_AddDouble(SphereConstraintObject, 2, Constraint.SphereConstraint->Radius);
	}

	//option<CylinderConstraint> cylinder_constraint = 2;
	if (Constraint.CylinderConstraint.IsSet())
	{
		Schema_Object* CylinderConstraintObject = Schema_AddObject(QueryConstraintObject, 2);

		AddCoordinateToSchema(CylinderConstraintObject, 1, Constraint.CylinderConstraint->Center);
		Schema_AddDouble(CylinderConstraintObject, 2, Constraint.CylinderConstraint->Radius);
	}

	//option<BoxConstraint> box_constraint = 3;
	if (Constraint.BoxConstraint.IsSet())
	{
		Schema_Object* BoxConstraintObject = Schema_AddObject(QueryConstraintObject, 3);
		AddCoordinateToSchema(BoxConstraintObject, 1, Constraint.BoxConstraint->Center);
		AddCoordinateToSchema(BoxConstraintObject, 2, Constraint.BoxConstraint->EdgeLength);
	}

	//option<RelativeSphereConstraint> relative_sphere_constraint = 4;
	if (Constraint.RelativeSphereConstraint.IsSet())
	{
		Schema_Object* RelativeSphereConstraintObject = Schema_AddObject(QueryConstraintObject, 4);

		Schema_AddDouble(RelativeSphereConstraintObject, 1, Constraint.RelativeSphereConstraint->Radius);
	}

	//option<RelativeCylinderConstraint> relative_cylinder_constraint = 5;
	if (Constraint.RelativeCylinderConstraint.IsSet())
	{
		Schema_Object* RelativeCylinderConstraintObject = Schema_AddObject(QueryConstraintObject, 5);
		Schema_AddDouble(RelativeCylinderConstraintObject, 1, Constraint.RelativeCylinderConstraint->Radius);
	}

	//option<RelativeBoxConstraint> relative_box_constraint = 6;
	if (Constraint.RelativeBoxConstraint.IsSet())
	{
		Schema_Object* RelativeBoxConstraintObject = Schema_AddObject(QueryConstraintObject, 6);
		AddCoordinateToSchema(RelativeBoxConstraintObject, 1, Constraint.RelativeBoxConstraint->EdgeLength);
	}

	//option<int64> entity_id_constraint = 7;
	if (Constraint.EntityIdConstraint.IsSet())
	{
		Schema_Object* EntityIdConstraintObject = Schema_AddObject(QueryConstraintObject, 7);
		Schema_AddInt64(EntityIdConstraintObject, 1, *Constraint.EntityIdConstraint);
	}

	//option<uint32> component_constraint = 8;
	if (Constraint.ComponentConstraint)
	{
		Schema_Object* ComponentConstraintObject = Schema_AddObject(QueryConstraintObject, 8);
		Schema_AddUint32(ComponentConstraintObject, 1, *Constraint.ComponentConstraint);
	}

	//list<QueryConstraint> and_constraint = 9;
	if (Constraint.AndConstraint.Num() > 0)
	{
		Schema_Object* AndConstraintObject = Schema_AddObject(QueryConstraintObject, 9);

		for (const ComponentInterest::QueryConstraint& AndConstraintEntry : Constraint.AndConstraint)
		{
			AddQueryConstraintToSchema(AndConstraintObject, 1, AndConstraintEntry);
		}
	}

	//list<QueryConstraint> or_constraint = 10;
	if (Constraint.OrConstraint.Num() > 0)
	{
		Schema_Object* OrConstraintObject = Schema_AddObject(QueryConstraintObject, 10);

		for (const ComponentInterest::QueryConstraint& OrConstraintEntry : Constraint.OrConstraint)
		{
			AddQueryConstraintToSchema(OrConstraintObject, 1, OrConstraintEntry);
		}
	}
}

inline void AddQueryToSchema(Schema_Object* Object, Schema_FieldId Id, const ComponentInterest::Query& Query)
{
	checkf(!(Query.FullSnapshotResult.IsSet() && Query.ResultComponentId.Num() > 0), TEXT("Either full_snapshot_result or a list of result_component_id should be provided. Providing both is invalid."));

	Schema_Object* QueryObject = Schema_AddObject(Object, Id);

	// Write the query constraint
	AddQueryConstraintToSchema(QueryObject, 1, Query.Constraint);

	//write the snapshot option
	if (Query.FullSnapshotResult.IsSet())
	{
		Schema_Object* FullSnapshotResultObject = Schema_AddObject(QueryObject, 2);
		Schema_AddBool(FullSnapshotResultObject, 1, *Query.FullSnapshotResult);
	}

	// write the resulting component if
	Schema_Object* ResultComponentIdObject = Schema_AddObject(QueryObject, 3);

	for (uint32 ComponentId : Query.ResultComponentId)
	{
		Schema_AddUint32(ResultComponentIdObject, 1, ComponentId);
	}
	// write option for frequency
	if (Query.Frequency)
	{
		Schema_Object* FrequencyObject = Schema_AddObject(QueryObject, 4);
		Schema_AddFloat(FrequencyObject, 1, *Query.Frequency);
	}
}

inline void AddComponentInterestToSchema(Schema_Object* Object, Schema_FieldId Id, const ComponentInterest& Value)
{
	Schema_Object* ComponentInterestObject = Schema_AddObject(Object, Id);

	for (const ComponentInterest::Query& QueryEntry : Value.Queries)
	{
		AddQueryToSchema(ComponentInterestObject, 1, QueryEntry);
	}
}

inline ComponentInterest::QueryConstraint IndexQueryConstraintFromSchema(Schema_Object* Object, Schema_FieldId Id, uint32 Index)
{
	ComponentInterest::QueryConstraint NewQueryConstraint;

	Schema_Object* QueryConstraintObject = Schema_IndexObject(Object, Id, Index);

	// option<SphereConstraint> sphere_constraint = 1;
	if (Schema_GetObjectCount(QueryConstraintObject, 1) > 0)
	{
		Schema_Object* SphereConstraintObject = Schema_GetObject(QueryConstraintObject, 1);

		NewQueryConstraint.SphereConstraint->Center = GetCoordinateFromSchema(SphereConstraintObject, 1);
		NewQueryConstraint.SphereConstraint->Radius = Schema_GetDouble(SphereConstraintObject, 2);
	}

	// option<CylinderConstraint> cylinder_constraint = 2;
	if (Schema_GetObjectCount(QueryConstraintObject, 2) > 0)
	{
		Schema_Object* CylinderConstraintObject = Schema_GetObject(QueryConstraintObject, 2);

		NewQueryConstraint.CylinderConstraint->Center = GetCoordinateFromSchema(CylinderConstraintObject, 1);
		NewQueryConstraint.CylinderConstraint->Radius = Schema_GetDouble(CylinderConstraintObject, 2);
	}

	// option<BoxConstraint> box_constraint = 3;
	if (Schema_GetObjectCount(QueryConstraintObject, 3) > 0)
	{
		Schema_Object* BoxConstraintObject = Schema_GetObject(QueryConstraintObject, 3);

		NewQueryConstraint.BoxConstraint->Center = GetCoordinateFromSchema(BoxConstraintObject, 1);
		NewQueryConstraint.BoxConstraint->EdgeLength = GetCoordinateFromSchema(BoxConstraintObject, 2);
	}

	// option<RelativeSphereConstraint> relative_sphere_constraint = 4;
	if (Schema_GetObjectCount(QueryConstraintObject, 4) > 0)
	{
		Schema_Object* RelativeSphereConstraintObject = Schema_GetObject(QueryConstraintObject, 4);

		NewQueryConstraint.RelativeSphereConstraint->Radius = Schema_GetDouble(RelativeSphereConstraintObject, 1);
	}

	// option<RelativeCylinderConstraint> relative_cylinder_constraint = 5;
	if (Schema_GetObjectCount(QueryConstraintObject, 5) > 0)
	{
		Schema_Object* RelativeCylinderConstraintObject = Schema_GetObject(QueryConstraintObject, 5);

		NewQueryConstraint.RelativeCylinderConstraint->Radius = Schema_GetDouble(RelativeCylinderConstraintObject, 1);
	}

	// option<RelativeBoxConstraint> relative_box_constraint = 6;
	if (Schema_GetObjectCount(QueryConstraintObject, 6) > 0)
	{
		Schema_Object* RelativeBoxConstraintObject = Schema_GetObject(QueryConstraintObject, 6);

		NewQueryConstraint.RelativeBoxConstraint->EdgeLength = GetCoordinateFromSchema(RelativeBoxConstraintObject, 1);
	}

	//option<int64> entity_id_constraint = 7;
	if (Schema_GetObjectCount(QueryConstraintObject, 7) > 0)
	{
		Schema_Object* EntityIdConstraintObject = Schema_GetObject(QueryConstraintObject, 7);

		NewQueryConstraint.EntityIdConstraint = Schema_GetInt64(EntityIdConstraintObject, 1);
	}

	// option<uint32> component_constraint = 8;
	if (Schema_GetObjectCount(QueryConstraintObject, 8) > 0)
	{
		Schema_Object* ComponentConstraintObject = Schema_GetObject(QueryConstraintObject, 8);

		NewQueryConstraint.EntityIdConstraint = Schema_GetUint32(ComponentConstraintObject, 1);
	}

	// list<QueryConstraint> and_constraint = 9;
	const uint32 AndConstraintCount = Schema_GetObjectCount(QueryConstraintObject, 9);
	NewQueryConstraint.AndConstraint.Reserve(AndConstraintCount);

	for (uint32 AndIndex = 0; AndIndex < AndConstraintCount; AndIndex++)
	{
		NewQueryConstraint.AndConstraint.Add(IndexQueryConstraintFromSchema(QueryConstraintObject, 9, AndIndex));
	}

	// list<QueryConstraint> or_constraint = 10;
	const uint32 OrConstraintCount = Schema_GetObjectCount(QueryConstraintObject, 10);
	NewQueryConstraint.OrConstraint.Reserve(OrConstraintCount);

	for (uint32 OrIndex = 0; OrIndex < OrConstraintCount; OrIndex++)
	{
		NewQueryConstraint.OrConstraint.Add(IndexQueryConstraintFromSchema(QueryConstraintObject, 10, OrIndex));
	}

	return NewQueryConstraint;
}

inline ComponentInterest::QueryConstraint GetQueryConstraintFromSchema(Schema_Object* Object, Schema_FieldId Id)
{
	return IndexQueryConstraintFromSchema(Object, Id, 1);
}

inline ComponentInterest::Query IndexQueryFromSchema(Schema_Object* Object, Schema_FieldId Id, uint32 Index)
{
	ComponentInterest::Query NewQuery;

	Schema_Object* QueryObject = Schema_IndexObject(Object, Id, Index);

	NewQuery.Constraint = GetQueryConstraintFromSchema(QueryObject, 1);

	if (Schema_GetObjectCount(QueryObject, 2) > 0)
	{
		NewQuery.FullSnapshotResult = !!Schema_GetBool(QueryObject, 2);
	}

	uint32 ResultComponentIdCount = Schema_GetObjectCount(QueryObject, 3);
	NewQuery.ResultComponentId.Reserve(ResultComponentIdCount);
	for (uint32 ComponentIdIndex = 0; ComponentIdIndex < ResultComponentIdCount; ComponentIdIndex++)
	{
		NewQuery.ResultComponentId.Add(Schema_IndexUint32(QueryObject, 3, ComponentIdIndex));
	}

	if (Schema_GetObjectCount(QueryObject, 4) > 0)
	{
		NewQuery.Frequency = Schema_GetFloat(QueryObject, 4);
	}

	return NewQuery;
}

inline ComponentInterest GetComponentInterestFromSchema(Schema_Object* Object, Schema_FieldId Id)
{
	ComponentInterest NewComponentInterest;

	Schema_Object* ComponentInterestObject = Schema_GetObject(Object, Id);

	uint32 QueryCount = Schema_GetObjectCount(ComponentInterestObject, 1);

	for (uint32 QueryIndex = 0; QueryIndex < QueryCount; QueryIndex++)
	{
		NewComponentInterest.Queries.Add(IndexQueryFromSchema(ComponentInterestObject, 1, QueryIndex));
	}

	return NewComponentInterest;
}

struct Interest : Component
{
	static const Worker_ComponentId ComponentId = SpatialConstants::INTEREST_COMPONENT_ID;

	Interest() = default;

	Interest(const Worker_ComponentData& Data)
	{
		Schema_Object* ComponentObject = Schema_GetComponentDataFields(Data.schema_type);

		uint32 KVPairCount = Schema_GetObjectCount(ComponentObject, 1);
		for (uint32 i = 0; i < KVPairCount; i++)
		{
			Schema_Object* KVPairObject = Schema_IndexObject(ComponentObject, 1, i);
			uint32 Key = Schema_GetUint32(KVPairObject, SCHEMA_MAP_KEY_FIELD_ID);
			improbable::ComponentInterest Value = GetComponentInterestFromSchema(KVPairObject, SCHEMA_MAP_VALUE_FIELD_ID);

			ComponentInterest.Add(Key, Value);
		}
	}

	bool IsEmpty()
	{
		return ComponentInterest.Num() == 0;
	}

	void ApplyComponentUpdate(const Worker_ComponentUpdate& Update)
	{
		Schema_Object* ComponentObject = Schema_GetComponentUpdateFields(Update.schema_type);

		// This is never emptied, so does not need an additional check for cleared fields
		uint32 KVPairCount = Schema_GetObjectCount(ComponentObject, 1);
		if (KVPairCount > 0)
		{
			ComponentInterest.Empty();
			for (uint32 i = 0; i < KVPairCount; i++)
			{
				Schema_Object* KVPairObject = Schema_IndexObject(ComponentObject, 1, i);
				uint32 Key = Schema_GetUint32(KVPairObject, SCHEMA_MAP_KEY_FIELD_ID);
				improbable::ComponentInterest Value = GetComponentInterestFromSchema(KVPairObject, SCHEMA_MAP_VALUE_FIELD_ID);

				ComponentInterest.Add(Key, Value);
			}
		}
	}

	Worker_ComponentData CreateInterestData()
	{
		Worker_ComponentData Data = {};
		Data.component_id = ComponentId;
		Data.schema_type = Schema_CreateComponentData(ComponentId);
		Schema_Object* ComponentObject = Schema_GetComponentDataFields(Data.schema_type);

		FillComponentData(ComponentObject);

		return Data;
	}

	Worker_ComponentUpdate CreateInterestUpdate()
	{
		Worker_ComponentUpdate ComponentUpdate = {};
		ComponentUpdate.component_id = ComponentId;
		ComponentUpdate.schema_type = Schema_CreateComponentUpdate(ComponentId);
		Schema_Object* ComponentObject = Schema_GetComponentUpdateFields(ComponentUpdate.schema_type);

		FillComponentData(ComponentObject);

		return ComponentUpdate;
	}

	void FillComponentData(Schema_Object* ComponentObject)
	{
		for (const auto& KVPair : ComponentInterest)
		{
			Schema_Object* KVPairObject = Schema_AddObject(ComponentObject, 1);
			Schema_AddUint32(KVPairObject, SCHEMA_MAP_KEY_FIELD_ID, KVPair.Key);
			AddComponentInterestToSchema(KVPairObject, SCHEMA_MAP_VALUE_FIELD_ID, KVPair.Value);
		}
	}

	TMap<uint32, ComponentInterest> ComponentInterest;
};
}