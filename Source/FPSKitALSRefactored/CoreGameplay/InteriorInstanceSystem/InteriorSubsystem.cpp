#include "InteriorSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../EventBusSystem/OutcomeConditionAsset.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractItemStatePayload.h"
#include "../InteractionSystem/InteractiveItemComponent.h"
#include "../InteractionSystem/InteractCommandPayload.h"
#include "../InteractionSystem/InteractSetEnabledPayload.h"
#include "../InteractionSystem/InteractSetRangePayload.h"
#include "../InteractionSystem/InteractSetTooltipPayload.h"
#include "FloorStatePayload.h"
#include "../MissionSystem/ReleaseMissionSnapshotPayload.h"
#include "../MissionSystem/MissionEnvelopePayload.h"
#include "../WorldStateSystem/WorldStateRecordPayload.h"
#include "FloorAssignmentComponent.h"
#include "UObject/UnrealType.h"
#include "EngineUtils.h"
#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "../LocationSystem/InteriorSetAsset.h"
#include "../LocationSystem/FloorAsset.h"
#include "../LocationSystem/LocationSpatialTypes.h"
#include "InteriorTransitionPayload.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerController_I.h"
#include "CoreBlueprintFunctionLibrary.h"
#include "../LocationSystem/LocationAnchorActor.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UpdateMissionListPayload.h"
#include "../SaveGame/GameSaveSubsystem.h"
#include "ApplyMissionCompletionPolicyPayload.h"
#include "SceneDataProvider.h"
#include <UpdateActiveMissionId.h>
#include "SpawnGroupSubsystem.h"
#include <LevelLoadedPayload.h>
#include "SpawnGroupSpawner.h"

/* Converts EFloorActorType to string (Преобразует EFloorActorType в строку) */
static FString ActorTypeToString(EFloorActorType Type);
/* Converts string to EFloorActorType (Преобразует строку в EFloorActorType) */
static EFloorActorType StringToActorType(const FString& Str);

// Removes mission snapshots from the map
// Удаляет снимки миссии из карты
auto RemoveMissionSnapshots = [](TMap<FInteriorFloorKey, TMap<FName, TArray<FFloorSavedActorState>>>& MissionSnapshotsMap,
	const FName& MissionId) -> bool
	{
		if (MissionId.IsNone()) return false;
		bool bRemovedAny = false;

		for (auto It = MissionSnapshotsMap.CreateIterator(); It; ++It)
		{
			TMap<FName, TArray<FFloorSavedActorState>>& PerFloorMap = It->Value;
			if (PerFloorMap.Remove(MissionId) > 0)
			{
				bRemovedAny = true;
				if (PerFloorMap.Num() == 0)
				{
					// If there are no missions left for the floor – remove the floor
					// Если для этажа не осталось миссий – удаляем этаж
					It.RemoveCurrent();
				}
			}
		}
		return bRemovedAny;
	};

// Determines whether an actor should be skipped based on envelope policy
// Определяет, нужно ли пропустить актора на основе политики конверта
static bool ShouldUseActor(const FMissionEnvelope& Envelope, EEnvelopeChannel ActorChannel, EMissionEndReason Reason, bool IsRead)
{
	// Look for entry by channel / Ищем запись для канала
	const FEnvelopeChannelEntry* FoundEntry = nullptr;
	bool IsResetPolicy = false;
	bool IsPartialPolicy = false;

	switch (Reason)
	{
	case EMissionEndReason::None:
	{
		IsResetPolicy = Envelope.RuntimePolicy == EJobSpacePolicy::Reset;
		IsPartialPolicy = Envelope.RuntimePolicy == EJobSpacePolicy::Partial;
		FoundEntry = Envelope.RuntimePolicyChannels.FindByPredicate(
			[ActorChannel](const FEnvelopeChannelEntry& Entry)
			{
				return Entry.Channel == ActorChannel;
			});
		break;
	}
	case EMissionEndReason::Completed:
	{
		IsResetPolicy = Envelope.NextStagePolicy == EJobSpacePolicy::Reset;
		IsPartialPolicy = Envelope.NextStagePolicy == EJobSpacePolicy::Partial;
		FoundEntry = Envelope.NextStagePolicyChannels.FindByPredicate(
			[ActorChannel](const FEnvelopeChannelEntry& Entry)
			{
				return Entry.Channel == ActorChannel;
			});
		break;
	}
	case EMissionEndReason::Failed:
	{
		IsResetPolicy = Envelope.MissionFailedPolicy == EJobSpacePolicy::Reset;
		IsPartialPolicy = Envelope.MissionFailedPolicy == EJobSpacePolicy::Partial;
		FoundEntry = Envelope.MissionFailedPolicyChannels.FindByPredicate(
			[ActorChannel](const FEnvelopeChannelEntry& Entry)
			{
				return Entry.Channel == ActorChannel;
			});
		break;
	}
	case EMissionEndReason::Abandoned:
	{
		IsResetPolicy = Envelope.MissionAbandonedPolicy == EJobSpacePolicy::Reset;
		IsPartialPolicy = Envelope.MissionAbandonedPolicy == EJobSpacePolicy::Partial;
		FoundEntry = Envelope.MissionAbandonedChannels.FindByPredicate(
			[ActorChannel](const FEnvelopeChannelEntry& Entry)
			{
				return Entry.Channel == ActorChannel;
			});
		break;
	}
	}

	if (IsRead || Reason != EMissionEndReason::None)
	{
		if (IsResetPolicy)
		{
			return false;
		}

		if (!FoundEntry)
		{
			if (IsPartialPolicy)
			{
				return false;
			}
			return true;
		}

		if (IsPartialPolicy && FoundEntry->Policy == EChannelPolicy::Freeze)
		{
			return true;
		}
		else
		{
			return !(IsResetPolicy || (IsPartialPolicy && FoundEntry->Policy == EChannelPolicy::Reset));
		}
	}
	else
	{
		return true;
	}

}

// Checks if the given actor ID exists in the mission-spawned actors for the specified floor
// Проверяет, существует ли указанный ID актора в спавненных миссией акторах для указанного этажа
auto ContainsActorIdInMissionSpawnedForFloor = [](const TMap<FInteriorFloorKey, TMap<FName, FFloorPopulationBuckets>>& MissionSpawnedMap,
	const FInteriorFloorKey& FloorKey,
	const FName& MissionId,
	const FGuid& ItemId) -> bool
	{
		if (!ItemId.IsValid()) return false;
		const auto* PerFloor = MissionSpawnedMap.Find(FloorKey);
		if (!PerFloor) return false;
		const auto* Buckets = PerFloor->Find(MissionId);
		if (!Buckets) return false;

		auto CheckArray = [&ItemId](const TArray<FFloorPopulationRecord>& Arr) -> bool
			{
				return Arr.ContainsByPredicate([&ItemId](const FFloorPopulationRecord& Rec)
					{
						return Rec.ActorId == ItemId;
					});
			};

		return CheckArray(Buckets->HeavyFurniture) ||
			CheckArray(Buckets->LightItems) ||
			CheckArray(Buckets->Terminals) ||
			CheckArray(Buckets->NPCSpawners) ||
			CheckArray(Buckets->Debris);
	};

// Removes a mission entry from the population map
// Удаляет запись о миссии из карты населения
auto RemoveMissionFromPopulationMap = [](TMap<FInteriorFloorKey, TMap<FName, FFloorPopulationBuckets>>& MissionMap, const FName& MissionId) -> bool
	{
		if (MissionId.IsNone()) return false;
		bool bRemovedAny = false;

		for (auto It = MissionMap.CreateIterator(); It; ++It)
		{
			TMap<FName, FFloorPopulationBuckets>& PerFloorMap = It->Value;
			if (PerFloorMap.Remove(MissionId) > 0)
			{
				bRemovedAny = true;
				if (PerFloorMap.Num() == 0)
				{
					// If no missions left for the floor – remove the floor key
					// Если для этажа не осталось миссий – удаляем ключ этажа
					It.RemoveCurrent();
				}
			}
		}
		return bRemovedAny;
	};

// Copies mission-spawned actors to the global spawned map
// Копирует спавненных миссией акторов в глобальную карту спавна
auto CopyMissionSpawnedToGlobal = [](TMap<FInteriorFloorKey, TMap<FName, FFloorPopulationBuckets>>& MissionSpawnedMap,
	TMap<FInteriorFloorKey, FFloorPopulationBuckets>& GlobalSpawnedMap,
	FMissionEnvelope Envelope,
	EMissionEndReason Reason,
	const FInteriorFloorKey& FloorKey,
	const FName& MissionId) -> bool
	{
		auto* PerFloor = MissionSpawnedMap.Find(FloorKey);
		if (!PerFloor) return false;
		auto* Buckets = PerFloor->Find(MissionId);
		if (!Buckets) return false;

		FFloorPopulationBuckets& GlobalBuckets = GlobalSpawnedMap.FindOrAdd(FloorKey);
		bool bCopied = false;

		auto AddUnique = [&](const TArray<FFloorPopulationRecord>& Source, TArray<FFloorPopulationRecord>& Dest)
			{
				for (const FFloorPopulationRecord& Rec : Source)
				{
					if (!Dest.ContainsByPredicate([&](const FFloorPopulationRecord& Ex) { return Ex.ActorId == Rec.ActorId; }))
					{
						Dest.Add(Rec);
						bCopied = true;
					}
				}
			};

		if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::HeavyFurniture), Reason, false))
			AddUnique(Buckets->HeavyFurniture, GlobalBuckets.HeavyFurniture);

		if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::LightItem), Reason, false))
			AddUnique(Buckets->LightItems, GlobalBuckets.LightItems);

		if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::Terminal), Reason, false))
			AddUnique(Buckets->Terminals, GlobalBuckets.Terminals);

		if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::SpawnGroupSpawner), Reason, false))
			AddUnique(Buckets->NPCSpawners, GlobalBuckets.NPCSpawners);

		if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::Debris), Reason, false))
			AddUnique(Buckets->Debris, GlobalBuckets.Debris);

		return bCopied;
	};

// Copies mission-destroyed actors to the global destroyed map
// Копирует уничтоженных миссией акторов в глобальную карту уничтоженных
auto CopyMissionDestroyedToGlobal = [](TMap<FInteriorFloorKey, TMap<FName, FFloorPopulationBuckets>>& MissionDestroyedMap,
	TMap<FInteriorFloorKey, FFloorPopulationBuckets>& GlobalDestroyedMap,
	FMissionEnvelope Envelope,
	EMissionEndReason Reason,
	const FInteriorFloorKey& FloorKey,
	const FName& MissionId) -> bool
	{
		auto* PerFloor = MissionDestroyedMap.Find(FloorKey);
		if (!PerFloor) return false;
		auto* Buckets = PerFloor->Find(MissionId);
		if (!Buckets) return false;

		FFloorPopulationBuckets& GlobalBuckets = GlobalDestroyedMap.FindOrAdd(FloorKey);
		bool bCopied = false;

		auto AddUnique = [&](const TArray<FFloorPopulationRecord>& Source, TArray<FFloorPopulationRecord>& Dest)
			{
				for (const FFloorPopulationRecord& Rec : Source)
				{
					if (!Dest.ContainsByPredicate([&](const FFloorPopulationRecord& Ex) { return Ex.ActorId == Rec.ActorId; }))
					{
						Dest.Add(Rec);
						bCopied = true;
					}
				}
			};

		if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::HeavyFurniture), Reason, false))
			AddUnique(Buckets->HeavyFurniture, GlobalBuckets.HeavyFurniture);

		if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::LightItem), Reason, false))
			AddUnique(Buckets->LightItems, GlobalBuckets.LightItems);

		if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::Terminal), Reason, false))
			AddUnique(Buckets->Terminals, GlobalBuckets.Terminals);

		if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::SpawnGroupSpawner), Reason, false))
			AddUnique(Buckets->NPCSpawners, GlobalBuckets.NPCSpawners);

		if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::Debris), Reason, false))
			AddUnique(Buckets->Debris, GlobalBuckets.Debris);

		return bCopied;
	};

// Removes an actor by ID from the mission-spawned map for a specific floor
// Удаляет актора по ID из карты спавна миссией для указанного этажа
auto RemoveFromMissionSpawnedByActorIdForFloor = [](TMap<FInteriorFloorKey, TMap<FName, FFloorPopulationBuckets>>& MissionMap,
	const FInteriorFloorKey& FloorKey,
	const FName& MissionId,
	const FGuid& ItemId) -> bool
	{
		if (!ItemId.IsValid()) return false;
		auto* PerFloor = MissionMap.Find(FloorKey);
		if (!PerFloor) return false;
		auto* Buckets = PerFloor->Find(MissionId);
		if (!Buckets) return false;

		bool bRemoved = false;
		auto RemoveFromArray = [&](TArray<FFloorPopulationRecord>& Arr)
			{
				int32 OldCount = Arr.Num();
				Arr.RemoveAll([&ItemId](const FFloorPopulationRecord& Rec) { return Rec.ActorId == ItemId; });
				if (OldCount != Arr.Num()) bRemoved = true;
			};
		RemoveFromArray(Buckets->HeavyFurniture);
		RemoveFromArray(Buckets->LightItems);
		RemoveFromArray(Buckets->Terminals);
		RemoveFromArray(Buckets->NPCSpawners);
		RemoveFromArray(Buckets->Debris);

		// If the bucket for this mission is empty – remove the mission entry
		// Если бакет для этой миссии опустел – удаляем запись о миссии
		if (Buckets->HeavyFurniture.IsEmpty() && Buckets->LightItems.IsEmpty() &&
			Buckets->Terminals.IsEmpty() && Buckets->NPCSpawners.IsEmpty() && Buckets->Debris.IsEmpty())
		{
			PerFloor->Remove(MissionId);
			// If no missions left for the floor – remove the floor key
			// Если для этажа не осталось миссий – удаляем ключ этажа
			if (PerFloor->Num() == 0)
				MissionMap.Remove(FloorKey);
		}
		return bRemoved;
	};

// Removes an actor by ID from the mission-destroyed map for a specific floor
// Удаляет актора по ID из карты уничтожения миссией для указанного этажа
auto RemoveFromMissionDestroyedByActorIdForFloor = [](TMap<FInteriorFloorKey, TMap<FName, FFloorPopulationBuckets>>& MissionDestroyedMap,
	const FInteriorFloorKey& FloorKey,
	const FName& MissionId,
	const FGuid& ItemId) -> bool
	{
		if (!ItemId.IsValid()) return false;

		// Find the map for the given floor / Находим карту для заданного этажа
		auto* PerFloor = MissionDestroyedMap.Find(FloorKey);
		if (!PerFloor) return false;

		// Find the buckets for the given mission / Находим бакеты для заданной миссии
		auto* Buckets = PerFloor->Find(MissionId);
		if (!Buckets) return false;

		bool bRemoved = false;

		// Lambda to remove from a specific array / Лямбда для удаления из конкретного массива
		auto RemoveFromArray = [&](TArray<FFloorPopulationRecord>& Arr)
			{
				int32 OldCount = Arr.Num();
				Arr.RemoveAll([&ItemId](const FFloorPopulationRecord& Rec) { return Rec.ActorId == ItemId; });
				if (OldCount != Arr.Num()) bRemoved = true;
			};

		RemoveFromArray(Buckets->HeavyFurniture);
		RemoveFromArray(Buckets->LightItems);
		RemoveFromArray(Buckets->Terminals);
		RemoveFromArray(Buckets->NPCSpawners);
		RemoveFromArray(Buckets->Debris);

		// If after removal all arrays for this mission are empty – remove the mission entry
		// Если после удаления все массивы для этой миссии пусты – удаляем запись о миссии
		if (Buckets->HeavyFurniture.IsEmpty() && Buckets->LightItems.IsEmpty() &&
			Buckets->Terminals.IsEmpty() && Buckets->NPCSpawners.IsEmpty() && Buckets->Debris.IsEmpty())
		{
			PerFloor->Remove(MissionId);
			// If no missions left for the floor – remove the floor key
			// Если для этажа не осталось миссий – удаляем ключ этажа
			if (PerFloor->Num() == 0)
				MissionDestroyedMap.Remove(FloorKey);
		}

		return bRemoved;
	};

// -----------------------------------------------------------------------------
// Helper functions for persistent population (channel filtering)
// Вспомогательные функции для постоянного населения (фильтрация по каналам)
// -----------------------------------------------------------------------------

// Copies records for a specific channel / Копирует записи для указанного канала
static void CopyRecordsForChannelHelper(const TArray<FFloorPopulationRecord>& Source, TArray<FFloorPopulationRecord>& Dest, EEnvelopeChannel Channel)
{
	for (const FFloorPopulationRecord& Rec : Source)
	{
		if (FloorActorTypeToEnvelopeChannel(Rec.ActorType) == Channel)
		{
			if (!Dest.ContainsByPredicate([&](const FFloorPopulationRecord& Existing) { return Existing.ActorId == Rec.ActorId; }))
				Dest.Add(Rec);
		}
	}
}

// Removes records for a specific channel / Удаляет записи для указанного канала
static void RemoveRecordsForChannelHelper(TArray<FFloorPopulationRecord>& Records, EEnvelopeChannel Channel)
{
	Records.RemoveAll([Channel](const FFloorPopulationRecord& Rec) {
		return FloorActorTypeToEnvelopeChannel(Rec.ActorType) == Channel;
		});
}

// Checks if an actor ID exists in the spawned map / Проверяет, существует ли ID актора в карте спавна
auto ContainsActorIdInSpawned = [](const TMap<FInteriorFloorKey, FFloorPopulationBuckets>& SpawnedMap, const FGuid& TargetActorId) -> bool
	{
		if (!TargetActorId.IsValid()) return false;

		for (const auto& Pair : SpawnedMap)
		{
			const FFloorPopulationBuckets& Buckets = Pair.Value;
			auto CheckArray = [&TargetActorId](const TArray<FFloorPopulationRecord>& Arr) -> bool
				{
					return Arr.ContainsByPredicate([&TargetActorId](const FFloorPopulationRecord& Record)
						{
							return Record.ActorId == TargetActorId;
						});
				};
			if (CheckArray(Buckets.HeavyFurniture)) return true;
			if (CheckArray(Buckets.LightItems)) return true;
			if (CheckArray(Buckets.Terminals)) return true;
			if (CheckArray(Buckets.NPCSpawners)) return true;
			if (CheckArray(Buckets.Debris)) return true;
		}
		return false;

	};

// Checks if a newly spawned actor ID exists in the spawned map
// Проверяет, существует ли ID нового спавненного актора в карте спавна
auto ContainsActorIdInSpawnedNew = [](const TMap<FInteriorFloorKey, FFloorPopulationBuckets>& SpawnedMap, const FGuid& TargetActorId) -> bool
	{
		if (!TargetActorId.IsValid()) return false;

		for (const auto& Pair : SpawnedMap)
		{
			const FFloorPopulationBuckets& Buckets = Pair.Value;
			auto CheckArray = [&TargetActorId](const TArray<FFloorPopulationRecord>& Arr) -> bool
				{
					return Arr.ContainsByPredicate([&TargetActorId](const FFloorPopulationRecord& Record)
						{
							return Record.ActorId == TargetActorId && Record.IsNewObject;
						});
				};
			if (CheckArray(Buckets.HeavyFurniture)) return true;
			if (CheckArray(Buckets.LightItems)) return true;
			if (CheckArray(Buckets.Terminals)) return true;
			if (CheckArray(Buckets.NPCSpawners)) return true;
			if (CheckArray(Buckets.Debris)) return true;
		}
		return false;

	};

// -----------------------------------------------------------------------------
// JSON serialization helpers for snapshot structures
// Вспомогательные функции сериализации JSON для структур снимков
// -----------------------------------------------------------------------------

// Converts FTransform to JSON object / Преобразует FTransform в JSON-объект
static TSharedPtr<FJsonObject> TransformToJsonObject(const FTransform& Transform)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	FVector Loc = Transform.GetLocation();
	FRotator Rot = Transform.Rotator();
	FVector Scale = Transform.GetScale3D();
	Obj->SetArrayField(TEXT("Location"), {
		MakeShared<FJsonValueNumber>(Loc.X),
		MakeShared<FJsonValueNumber>(Loc.Y),
		MakeShared<FJsonValueNumber>(Loc.Z)
		});
	Obj->SetArrayField(TEXT("Rotation"), {
		MakeShared<FJsonValueNumber>(Rot.Pitch),
		MakeShared<FJsonValueNumber>(Rot.Yaw),
		MakeShared<FJsonValueNumber>(Rot.Roll)
		});
	Obj->SetArrayField(TEXT("Scale"), {
		MakeShared<FJsonValueNumber>(Scale.X),
		MakeShared<FJsonValueNumber>(Scale.Y),
		MakeShared<FJsonValueNumber>(Scale.Z)
		});
	return Obj;
}

// Creates FTransform from JSON object / Создает FTransform из JSON-объекта
static FTransform TransformFromJsonObject(const TSharedPtr<FJsonObject>& Obj)
{
	if (!Obj.IsValid()) return FTransform::Identity;
	auto ReadVector = [](const TSharedPtr<FJsonObject>& O, const FString& Field) -> FVector
		{
			if (!O->HasField(Field)) return FVector::ZeroVector;
			const TArray<TSharedPtr<FJsonValue>>& Arr = O->GetArrayField(Field);
			if (Arr.Num() >= 3)
				return FVector(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
			return FVector::ZeroVector;
		};
	FVector Loc = ReadVector(Obj, TEXT("Location"));
	FRotator Rot;
	if (Obj->HasField(TEXT("Rotation")))
	{
		const TArray<TSharedPtr<FJsonValue>>& Arr = Obj->GetArrayField(TEXT("Rotation"));
		if (Arr.Num() >= 3)
			Rot = FRotator(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
	}
	FVector Scale = ReadVector(Obj, TEXT("Scale"));
	return FTransform(Rot, Loc, Scale);
}

// Converts property entry to JSON / Преобразует запись свойства в JSON
static TSharedPtr<FJsonObject> PropertyEntryToJson(const FFloorSavedPropertyEntry& Entry)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("PropertyName"), Entry.PropertyName.ToString());
	Obj->SetStringField(TEXT("ValueText"), Entry.ValueText);
	return Obj;
}

// Creates property entry from JSON / Создает запись свойства из JSON
static FFloorSavedPropertyEntry PropertyEntryFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FFloorSavedPropertyEntry Entry;
	if (Obj.IsValid())
	{
		Entry.PropertyName = FName(*Obj->GetStringField(TEXT("PropertyName")));
		Entry.ValueText = Obj->GetStringField(TEXT("ValueText"));
	}
	return Entry;
}

// Converts component state to JSON / Преобразует состояние компонента в JSON
static TSharedPtr<FJsonObject> ComponentStateToJson(const FFloorSavedComponentState& State)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("ComponentName"), State.ComponentName.ToString());
	Obj->SetStringField(TEXT("ComponentClassName"), State.ComponentClassName.ToString());
	Obj->SetBoolField(TEXT("bWasActive"), State.bWasActive);
	Obj->SetBoolField(TEXT("bWasAttached"), State.bWasAttached);
	Obj->SetStringField(TEXT("AttachParentName"), State.AttachParentName.ToString());
	Obj->SetStringField(TEXT("AttachSocketName"), State.AttachSocketName.ToString());
	Obj->SetObjectField(TEXT("RelativeTransform"), TransformToJsonObject(State.RelativeTransform));
	Obj->SetObjectField(TEXT("WorldTransform"), TransformToJsonObject(State.WorldTransform));
	Obj->SetBoolField(TEXT("bHasRelativeTransform"), State.bHasRelativeTransform);
	Obj->SetBoolField(TEXT("bHasWorldTransform"), State.bHasWorldTransform);
	Obj->SetBoolField(TEXT("bWasSimulatingPhysics"), State.bWasSimulatingPhysics);
	Obj->SetArrayField(TEXT("SavedLinearVelocity"), {
		MakeShared<FJsonValueNumber>(State.SavedLinearVelocity.X),
		MakeShared<FJsonValueNumber>(State.SavedLinearVelocity.Y),
		MakeShared<FJsonValueNumber>(State.SavedLinearVelocity.Z)
		});
	Obj->SetArrayField(TEXT("SavedAngularVelocityDeg"), {
		MakeShared<FJsonValueNumber>(State.SavedAngularVelocityDeg.X),
		MakeShared<FJsonValueNumber>(State.SavedAngularVelocityDeg.Y),
		MakeShared<FJsonValueNumber>(State.SavedAngularVelocityDeg.Z)
		});

	TArray<TSharedPtr<FJsonValue>> PropsArray;
	for (const auto& Prop : State.Properties)
		PropsArray.Add(MakeShared<FJsonValueObject>(PropertyEntryToJson(Prop)));
	Obj->SetArrayField(TEXT("Properties"), PropsArray);
	return Obj;
}

// Creates component state from JSON / Создает состояние компонента из JSON
static FFloorSavedComponentState ComponentStateFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FFloorSavedComponentState State;
	if (!Obj.IsValid()) return State;
	State.ComponentName = FName(*Obj->GetStringField(TEXT("ComponentName")));
	State.ComponentClassName = FName(*Obj->GetStringField(TEXT("ComponentClassName")));
	State.bWasActive = Obj->GetBoolField(TEXT("bWasActive"));
	State.bWasAttached = Obj->GetBoolField(TEXT("bWasAttached"));
	State.AttachParentName = FName(*Obj->GetStringField(TEXT("AttachParentName")));
	State.AttachSocketName = FName(*Obj->GetStringField(TEXT("AttachSocketName")));
	if (Obj->HasField(TEXT("RelativeTransform")))
		State.RelativeTransform = TransformFromJsonObject(Obj->GetObjectField(TEXT("RelativeTransform")));
	if (Obj->HasField(TEXT("WorldTransform")))
		State.WorldTransform = TransformFromJsonObject(Obj->GetObjectField(TEXT("WorldTransform")));
	State.bHasRelativeTransform = Obj->GetBoolField(TEXT("bHasRelativeTransform"));
	State.bHasWorldTransform = Obj->GetBoolField(TEXT("bHasWorldTransform"));
	State.bWasSimulatingPhysics = Obj->GetBoolField(TEXT("bWasSimulatingPhysics"));

	if (Obj->HasField(TEXT("SavedLinearVelocity")))
	{
		const TArray<TSharedPtr<FJsonValue>>& Arr = Obj->GetArrayField(TEXT("SavedLinearVelocity"));
		if (Arr.Num() >= 3)
			State.SavedLinearVelocity = FVector(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
	}
	if (Obj->HasField(TEXT("SavedAngularVelocityDeg")))
	{
		const TArray<TSharedPtr<FJsonValue>>& Arr = Obj->GetArrayField(TEXT("SavedAngularVelocityDeg"));
		if (Arr.Num() >= 3)
			State.SavedAngularVelocityDeg = FVector(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
	}

	if (Obj->HasField(TEXT("Properties")))
	{
		const TArray<TSharedPtr<FJsonValue>>& PropsArray = Obj->GetArrayField(TEXT("Properties"));
		for (const auto& Val : PropsArray)
			if (Val->Type == EJson::Object)
				State.Properties.Add(PropertyEntryFromJson(Val->AsObject()));
	}
	return State;
}

// Converts actor state to JSON / Преобразует состояние актора в JSON
static TSharedPtr<FJsonObject> ActorStateToJson(const FFloorSavedActorState& State)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("ActorName"), State.ActorName);
	Obj->SetStringField(TEXT("ItemId"), State.ItemId.ToString());
	Obj->SetStringField(TEXT("ActorType"), ActorTypeToString(State.ActorType));
	Obj->SetObjectField(TEXT("ActorTransform"), TransformToJsonObject(State.ActorTransform));
	Obj->SetObjectField(TEXT("RelativeTransform"), TransformToJsonObject(State.RelativeTransform));
	Obj->SetBoolField(TEXT("bHasRelativeTransform"), State.bHasRelativeTransform);

	TArray<TSharedPtr<FJsonValue>> PropsArray;
	for (const auto& Prop : State.ActorProperties)
		PropsArray.Add(MakeShared<FJsonValueObject>(PropertyEntryToJson(Prop)));
	Obj->SetArrayField(TEXT("ActorProperties"), PropsArray);

	TArray<TSharedPtr<FJsonValue>> CompArray;
	for (const auto& Comp : State.ComponentStates)
		CompArray.Add(MakeShared<FJsonValueObject>(ComponentStateToJson(Comp)));
	Obj->SetArrayField(TEXT("ComponentStates"), CompArray);
	return Obj;
}

// Creates actor state from JSON / Создает состояние актора из JSON
static FFloorSavedActorState ActorStateFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FFloorSavedActorState State;
	if (!Obj.IsValid()) return State;

	Obj->TryGetStringField(TEXT("ActorName"), State.ActorName);
	FGuid::Parse(Obj->GetStringField(TEXT("ItemId")), State.ItemId);

	FString ActorTypeStr;
	if (Obj->TryGetStringField(TEXT("ActorType"), ActorTypeStr))
		State.ActorType = StringToActorType(ActorTypeStr);

	if (Obj->HasField(TEXT("ActorTransform")))
		State.ActorTransform = TransformFromJsonObject(Obj->GetObjectField(TEXT("ActorTransform")));
	if (Obj->HasField(TEXT("RelativeTransform")))
		State.RelativeTransform = TransformFromJsonObject(Obj->GetObjectField(TEXT("RelativeTransform")));
	State.bHasRelativeTransform = Obj->GetBoolField(TEXT("bHasRelativeTransform"));

	if (Obj->HasField(TEXT("ActorProperties")))
	{
		const TArray<TSharedPtr<FJsonValue>>& PropsArray = Obj->GetArrayField(TEXT("ActorProperties"));
		for (const auto& Val : PropsArray)
			if (Val->Type == EJson::Object)
				State.ActorProperties.Add(PropertyEntryFromJson(Val->AsObject()));
	}
	if (Obj->HasField(TEXT("ComponentStates")))
	{
		const TArray<TSharedPtr<FJsonValue>>& CompArray = Obj->GetArrayField(TEXT("ComponentStates"));
		for (const auto& Val : CompArray)
			if (Val->Type == EJson::Object)
				State.ComponentStates.Add(ComponentStateFromJson(Val->AsObject()));
	}
	return State;
}

// Serializes floor state snapshots to JSON / Сериализует снимки состояния этажа в JSON
static TSharedPtr<FJsonValue> SerializeFloorStateSnapshots(const TMap<FInteriorFloorKey, TArray<FFloorSavedActorState>>& Snapshots)
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	for (const auto& Pair : Snapshots)
	{
		FString KeyStr = FString::Printf(TEXT("%s|%s"), *Pair.Key.InteriorSetId.ToString(), *Pair.Key.FloorId.ToString());
		TArray<TSharedPtr<FJsonValue>> Arr;
		for (const auto& State : Pair.Value)
			Arr.Add(MakeShared<FJsonValueObject>(ActorStateToJson(State)));
		Root->SetArrayField(KeyStr, Arr);
	}
	return MakeShared<FJsonValueObject>(Root);
}

// Deserializes floor state snapshots from JSON / Десериализует снимки состояния этажа из JSON
static void DeserializeFloorStateSnapshots(const TSharedPtr<FJsonValue>& JsonValue, TMap<FInteriorFloorKey, TArray<FFloorSavedActorState>>& OutSnapshots)
{
	OutSnapshots.Empty();
	if (!JsonValue.IsValid() || JsonValue->Type != EJson::Object) return;
	TSharedPtr<FJsonObject> Root = JsonValue->AsObject();
	for (const auto& Pair : Root->Values)
	{
		TArray<FString> Parts;
		Pair.Key.ParseIntoArray(Parts, TEXT("|"), false);
		if (Parts.Num() != 2) continue;
		FGuid InteriorSetId, FloorId;
		if (!FGuid::Parse(Parts[0], InteriorSetId) || !FGuid::Parse(Parts[1], FloorId)) continue;
		FInteriorFloorKey Key(InteriorSetId, FloorId);
		TArray<FFloorSavedActorState> States;
		if (Pair.Value->Type == EJson::Array)
		{
			const TArray<TSharedPtr<FJsonValue>>& Arr = Pair.Value->AsArray();
			for (const auto& Item : Arr)
				if (Item->Type == EJson::Object)
					States.Add(ActorStateFromJson(Item->AsObject()));
		}
		OutSnapshots.Add(Key, MoveTemp(States));
	}
}

// Serializes mission floor snapshots to JSON / Сериализует снимки этажей миссий в JSON
static TSharedPtr<FJsonValue> SerializeMissionFloorSnapshots(const TMap<FInteriorFloorKey, TMap<FName, TArray<FFloorSavedActorState>>>& Snapshots)
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	for (const auto& Pair : Snapshots)
	{
		FString KeyStr = FString::Printf(TEXT("%s|%s"), *Pair.Key.InteriorSetId.ToString(), *Pair.Key.FloorId.ToString());
		TSharedPtr<FJsonObject> FloorObj = MakeShared<FJsonObject>();
		for (const auto& MissionPair : Pair.Value)
		{
			TArray<TSharedPtr<FJsonValue>> Arr;
			for (const auto& State : MissionPair.Value)
				Arr.Add(MakeShared<FJsonValueObject>(ActorStateToJson(State)));
			FloorObj->SetArrayField(MissionPair.Key.ToString(), Arr);
		}
		Root->SetObjectField(KeyStr, FloorObj);
	}
	return MakeShared<FJsonValueObject>(Root);
}

// Deserializes mission floor snapshots from JSON / Десериализует снимки этажей миссий из JSON
static void DeserializeMissionFloorSnapshots(const TSharedPtr<FJsonValue>& JsonValue, TMap<FInteriorFloorKey, TMap<FName, TArray<FFloorSavedActorState>>>& OutSnapshots)
{
	OutSnapshots.Empty();
	if (!JsonValue.IsValid() || JsonValue->Type != EJson::Object) return;
	TSharedPtr<FJsonObject> Root = JsonValue->AsObject();
	for (const auto& Pair : Root->Values)
	{
		TArray<FString> Parts;
		Pair.Key.ParseIntoArray(Parts, TEXT("|"), false);
		if (Parts.Num() != 2) continue;
		FGuid InteriorSetId, FloorId;
		if (!FGuid::Parse(Parts[0], InteriorSetId) || !FGuid::Parse(Parts[1], FloorId)) continue;
		FInteriorFloorKey Key(InteriorSetId, FloorId);
		TMap<FName, TArray<FFloorSavedActorState>> PerMission;
		if (Pair.Value->Type == EJson::Object)
		{
			TSharedPtr<FJsonObject> FloorObj = Pair.Value->AsObject();
			for (const auto& MissionPair : FloorObj->Values)
			{
				FName MissionId(*MissionPair.Key);
				TArray<FFloorSavedActorState> States;
				if (MissionPair.Value->Type == EJson::Array)
				{
					const TArray<TSharedPtr<FJsonValue>>& Arr = MissionPair.Value->AsArray();
					for (const auto& Item : Arr)
						if (Item->Type == EJson::Object)
							States.Add(ActorStateFromJson(Item->AsObject()));
				}
				PerMission.Add(MissionId, MoveTemp(States));
			}
		}
		OutSnapshots.Add(Key, MoveTemp(PerMission));
	}
}

// -----------------------------------------------------------------------------
// Snapshot helpers
// Вспомогательные функции для снимков
// -----------------------------------------------------------------------------
	// Normalization: take base filename without path, remove PIE prefixes, convert to lowercase
	// Нормализация: берём базовое имя файла без пути, убираем PIE-префиксы и приводим к нижнему регистру
static auto NormalizeLevelName = [](const FString& InPath) -> FString
	{
		if (InPath.IsEmpty()) return FString();
		FString PackagePath = InPath;
		if (PackagePath.Contains(TEXT(".")))
			PackagePath = FPackageName::ObjectPathToPackageName(PackagePath);
		FString Base = FPaths::GetBaseFilename(PackagePath);
		// Remove PIE prefixes like UEDPIE_0_
		// Убираем PIE-префиксы вида UEDPIE_0_
		int32 PIEPos = Base.Find(TEXT("UEDPIE_"), ESearchCase::IgnoreCase);
		if (PIEPos != INDEX_NONE)
		{
			int32 MPos = Base.Find(TEXT("_M_"), ESearchCase::IgnoreCase, ESearchDir::FromStart, PIEPos);
			if (MPos != INDEX_NONE && MPos + 3 < Base.Len())
				Base = Base.Mid(MPos + 3);
			else
				Base = Base.Mid(PIEPos + 6);
		}
		return Base.ToLower();
	};

// Collects properties marked with SaveGame flag
// Собирает свойства, помеченные флагом SaveGame
static void CollectSaveGameProperties(UObject* Obj, TArray<FFloorSavedPropertyEntry>& OutProps)
{
	if (!IsValid(Obj)) return;
	for (TFieldIterator<FProperty> It(Obj->GetClass()); It; ++It)
	{
		FProperty* Prop = *It;
		if (!Prop->HasAllPropertyFlags(CPF_SaveGame)) continue;

		FString Exported;
		Prop->ExportText_InContainer(0, Exported, Obj, nullptr, Obj, PPF_None);
		OutProps.Add({ Prop->GetFName(), Exported });
	}
}

// Applies saved properties to an object
// Применяет сохранённые свойства к объекту
static void ApplySaveGameProperties(UObject* Obj, const TArray<FFloorSavedPropertyEntry>& Props)
{
	if (!IsValid(Obj)) return;
	for (const FFloorSavedPropertyEntry& Entry : Props)
	{
		FProperty* Prop = FindFProperty<FProperty>(Obj->GetClass(), Entry.PropertyName);
		if (!Prop) continue;
		Prop->ImportText_InContainer(*Entry.ValueText, Obj, Obj, PPF_None);
	}
}

// Restores a scene component's transform
// Восстанавливает трансформацию компонента сцены
static void RestoreSceneComponentTransform(USceneComponent* Comp, const FFloorSavedComponentState& CState)
{
	if (!IsValid(Comp)) return;

	if (CState.bWasAttached)
	{
		AActor* Owner = Comp->GetOwner();
		if (IsValid(Owner))
		{
			for (UActorComponent* AC : Owner->GetComponents())
			{
				USceneComponent* ParentSC = Cast<USceneComponent>(AC);
				if (ParentSC && ParentSC->GetFName() == CState.AttachParentName)
				{
					Comp->AttachToComponent(ParentSC, FAttachmentTransformRules::KeepRelativeTransform, CState.AttachSocketName);
					break;
				}
			}
		}
	}

	if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comp))
	{
		const bool bWasSim = CState.bWasSimulatingPhysics;
		if (bWasSim)
		{
			Prim->SetSimulatePhysics(false);

			if (CState.bHasRelativeTransform)
				Comp->SetRelativeTransform(CState.RelativeTransform, false, nullptr, ETeleportType::TeleportPhysics);
			else if (CState.bHasWorldTransform && !CState.bWasAttached)
				Comp->SetWorldTransform(CState.WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);

			Prim->SetSimulatePhysics(true);
			Prim->SetPhysicsLinearVelocity(CState.SavedLinearVelocity);
			Prim->SetPhysicsAngularVelocityInDegrees(CState.SavedAngularVelocityDeg);
		}
		else
		{
			if (CState.bHasRelativeTransform)
				Comp->SetRelativeTransform(CState.RelativeTransform, false, nullptr, ETeleportType::None);
			else if (CState.bHasWorldTransform && !CState.bWasAttached)
				Comp->SetWorldTransform(CState.WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
	else
	{
		if (CState.bHasRelativeTransform)
			Comp->SetRelativeTransform(CState.RelativeTransform, false, nullptr, ETeleportType::None);
		else if (CState.bHasWorldTransform && !CState.bWasAttached)
			Comp->SetWorldTransform(CState.WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

// Captures a snapshot of the actor's state
// Создаёт снимок состояния актора
static void SnapshotActor(AActor* Actor, FFloorSavedActorState& OutSnapshot)
{
	if (!IsValid(Actor)) return;

	UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
	if (!Comp) return;

	OutSnapshot.ActorName = Actor->GetName();
	OutSnapshot.ActorType = Comp->ActorType;
	OutSnapshot.ActorTransform = Actor->GetActorTransform();
	CollectSaveGameProperties(Actor, OutSnapshot.ActorProperties);

	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);
	for (UActorComponent* C : Components)
	{
		if (!IsValid(C)) continue;
		TArray<FFloorSavedPropertyEntry> Props;
		CollectSaveGameProperties(C, Props);

		FFloorSavedComponentState CState;
		CState.ComponentName = C->GetFName();
		CState.ComponentClassName = C->GetClass()->GetFName();
		CState.bWasActive = C->IsActive();
		CState.Properties = MoveTemp(Props);

		if (USceneComponent* SC = Cast<USceneComponent>(C))
		{
			if (USceneComponent* ParentSC = SC->GetAttachParent())
			{
				CState.bWasAttached = true;
				CState.AttachParentName = ParentSC->GetFName();
				CState.AttachSocketName = SC->GetAttachSocketName();
				CState.RelativeTransform = SC->GetRelativeTransform();
				CState.bHasRelativeTransform = true;
			}
			CState.WorldTransform = SC->GetComponentTransform();
			CState.bHasWorldTransform = true;
		}

		if (UPrimitiveComponent* PC = Cast<UPrimitiveComponent>(C))
		{
			CState.bWasSimulatingPhysics = PC->IsSimulatingPhysics();
			if (CState.bWasSimulatingPhysics)
			{
				CState.SavedLinearVelocity = PC->GetPhysicsLinearVelocity();
				CState.SavedAngularVelocityDeg = PC->GetPhysicsAngularVelocityInDegrees();
			}
		}

		OutSnapshot.ComponentStates.Add(MoveTemp(CState));
	}
}

// Restores an actor from a snapshot
// Восстанавливает актора из снимка
static void RestoreActorSnapshot(AActor* Actor, const FFloorSavedActorState& Snapshot, bool bApplyWorldTransform = true, bool bForceRelative = false)
{
	if (!IsValid(Actor)) return;

	// 1. Set world transform if required
	// 1. Устанавливаем мировую трансформацию актора, если требуется
	if (bApplyWorldTransform)
	{
		Actor->SetActorTransform(Snapshot.ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		Actor->SetActorRelativeTransform(Snapshot.RelativeTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	// 2. Restore SaveGame properties of the actor itself
	// 2. Восстанавливаем SaveGame-свойства самого актора
	ApplySaveGameProperties(Actor, Snapshot.ActorProperties);

	// 3. Build a component map by name for quick access
	// 3. Строим карту компонентов по имени для быстрого доступа
	TMap<FName, UActorComponent*> ComponentsByName;
	for (UActorComponent* C : Actor->GetComponents())
	{
		if (!IsValid(C)) continue;
		ComponentsByName.Add(C->GetFName(), C);
	}

	// 4. First restore attachment for components with bWasAttached == true
	// 4. Сначала восстанавливаем аттачмент (привязку) для компонентов, у которых bWasAttached == true
	for (const FFloorSavedComponentState& CState : Snapshot.ComponentStates)
	{
		if (!CState.bWasAttached) continue;
		UActorComponent** Found = ComponentsByName.Find(CState.ComponentName);
		if (!Found || !IsValid(*Found)) continue;
		USceneComponent* TargetSC = Cast<USceneComponent>(*Found);
		if (!TargetSC) continue;

		UActorComponent** ParentFound = ComponentsByName.Find(CState.AttachParentName);
		if (ParentFound && IsValid(*ParentFound))
		{
			if (USceneComponent* ParentSC = Cast<USceneComponent>(*ParentFound))
				TargetSC->AttachToComponent(ParentSC, FAttachmentTransformRules::KeepRelativeTransform, CState.AttachSocketName);
		}
	}

	// 5. Restore component properties and transforms
	// 5. Восстанавливаем свойства и трансформации компонентов
	for (const FFloorSavedComponentState& CState : Snapshot.ComponentStates)
	{
		UActorComponent** Found = ComponentsByName.Find(CState.ComponentName);
		if (!Found || !IsValid(*Found)) continue;
		UActorComponent* Target = *Found;

		// Restore SaveGame properties
		// Восстанавливаем SaveGame-свойства компонента
		ApplySaveGameProperties(Target, CState.Properties);

		// Restore activity
		// Восстанавливаем активность
		if (Target->IsActive() != CState.bWasActive)
			CState.bWasActive ? Target->Activate(true) : Target->Deactivate();

		// Restore transform for SceneComponent
		// Восстанавливаем трансформацию для SceneComponent
		if (USceneComponent* SC = Cast<USceneComponent>(Target))
		{
			// If relative transform is forced and available – use it
			// Если принудительно требуем относительную трансформацию и она есть – используем её
			if (bForceRelative && CState.bHasRelativeTransform)
			{
				SC->SetRelativeTransform(CState.RelativeTransform, false, nullptr, ETeleportType::TeleportPhysics);
			}
			else
			{
				// Otherwise use standard logic (which may use world transform)
				// Иначе используем стандартную логику (которая может взять мировую трансформацию)
				RestoreSceneComponentTransform(SC, CState);
			}
		}
	}
}
// -----------------------------------------------------------------------------
// JSON serialization helpers for population structures
// Вспомогательные функции сериализации JSON для структур населения
// -----------------------------------------------------------------------------

// Converts EFloorActorType to string / Преобразует EFloorActorType в строку
static FString ActorTypeToString(EFloorActorType Type)
{
	switch (Type)
	{
	case EFloorActorType::HeavyFurniture: return TEXT("HeavyFurniture");
	case EFloorActorType::LightItem:      return TEXT("LightItem");
	case EFloorActorType::Debris:         return TEXT("Debris");
	case EFloorActorType::DoorLocks:      return TEXT("DoorLocks");
	case EFloorActorType::StableActor:    return TEXT("StableActor");
	case EFloorActorType::DialogueAccess: return TEXT("DialogueAccess");
	case EFloorActorType::Terminal:       return TEXT("Terminal");
	case EFloorActorType::InventoryItems: return TEXT("InventoryItems");
	case EFloorActorType::SpawnGroupSpawner:    return TEXT("NPC_Spawner");
	case EFloorActorType::LocationTriggers: return TEXT("LocationTriggers");
	default: return TEXT("LightItem");
	}
}

// Converts string to EFloorActorType / Преобразует строку в EFloorActorType
static EFloorActorType StringToActorType(const FString& Str)
{
	if (Str == TEXT("HeavyFurniture")) return EFloorActorType::HeavyFurniture;
	if (Str == TEXT("LightItem"))      return EFloorActorType::LightItem;
	if (Str == TEXT("Debris"))         return EFloorActorType::Debris;
	if (Str == TEXT("DoorLocks"))      return EFloorActorType::DoorLocks;
	if (Str == TEXT("StableActor"))    return EFloorActorType::StableActor;
	if (Str == TEXT("DialogueAccess")) return EFloorActorType::DialogueAccess;
	if (Str == TEXT("Terminal"))       return EFloorActorType::Terminal;
	if (Str == TEXT("InventoryItems")) return EFloorActorType::InventoryItems;
	if (Str == TEXT("NPC_Spawner"))    return EFloorActorType::SpawnGroupSpawner;
	if (Str == TEXT("LocationTriggers"))return EFloorActorType::LocationTriggers;
	return EFloorActorType::LightItem;
}

// Converts a population record to JSON / Преобразует запись о населении в JSON
static TSharedPtr<FJsonObject> PopulationRecordToJson(const FFloorPopulationRecord& Record)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("ActorType"), ActorTypeToString(Record.ActorType));
	if (Record.SourceClass)
		Obj->SetStringField(TEXT("SourceClass"), Record.SourceClass->GetPathName());
	Obj->SetStringField(TEXT("ActorId"), Record.ActorId.ToString());
	Obj->SetBoolField(TEXT("IsNewObject"), Record.IsNewObject);
	Obj->SetObjectField(TEXT("WorldTransform"), TransformToJsonObject(Record.WorldTransform));
	Obj->SetBoolField(TEXT("bHasAnchor"), Record.bHasAnchor);
	return Obj;
}

// Creates a population record from JSON / Создает запись о населении из JSON
static FFloorPopulationRecord PopulationRecordFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FFloorPopulationRecord Record;
	if (!Obj.IsValid()) return Record;

	FString ActorTypeStr;
	if (Obj->TryGetStringField(TEXT("ActorType"), ActorTypeStr))
		Record.ActorType = StringToActorType(ActorTypeStr);

	FString SourceClassPath;
	if (Obj->TryGetStringField(TEXT("SourceClass"), SourceClassPath))
		Record.SourceClass = LoadClass<AActor>(nullptr, *SourceClassPath);

	FGuid::Parse(Obj->GetStringField(TEXT("ActorId")), Record.ActorId);

	// Read IsNewObject field; if missing in save – default to true
	// Чтение поля IsNewObject, если его нет в сохранении – по умолчанию true
	if (!Obj->TryGetBoolField(TEXT("IsNewObject"), Record.IsNewObject))
		Record.IsNewObject = true;   // fallback added / добавлен fallback

	if (Obj->HasField(TEXT("WorldTransform")))
		Record.WorldTransform = TransformFromJsonObject(Obj->GetObjectField(TEXT("WorldTransform")));
	Obj->TryGetBoolField(TEXT("bHasAnchor"), Record.bHasAnchor);
	return Record;
}

// Converts population buckets to JSON / Преобразует бакеты населения в JSON
static TSharedPtr<FJsonObject> PopulationBucketsToJson(const FFloorPopulationBuckets& Buckets)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	auto SerializeArray = [](const TArray<FFloorPopulationRecord>& Arr) -> TArray<TSharedPtr<FJsonValue>>
		{
			TArray<TSharedPtr<FJsonValue>> Result;
			for (const auto& R : Arr)
				Result.Add(MakeShared<FJsonValueObject>(PopulationRecordToJson(R)));
			return Result;
		};
	Obj->SetArrayField(TEXT("HeavyFurniture"), SerializeArray(Buckets.HeavyFurniture));
	Obj->SetArrayField(TEXT("LightItems"), SerializeArray(Buckets.LightItems));
	Obj->SetArrayField(TEXT("Terminals"), SerializeArray(Buckets.Terminals));
	Obj->SetArrayField(TEXT("NPCSpawners"), SerializeArray(Buckets.NPCSpawners));
	Obj->SetArrayField(TEXT("Debris"), SerializeArray(Buckets.Debris));
	return Obj;
}

// Creates population buckets from JSON / Создает бакеты населения из JSON
static FFloorPopulationBuckets PopulationBucketsFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FFloorPopulationBuckets Buckets;
	if (!Obj.IsValid()) return Buckets;

	auto DeserializeArray = [](const TArray<TSharedPtr<FJsonValue>>& Arr) -> TArray<FFloorPopulationRecord>
		{
			TArray<FFloorPopulationRecord> Result;
			for (const auto& Val : Arr)
				if (Val->Type == EJson::Object)
					Result.Add(PopulationRecordFromJson(Val->AsObject()));
			return Result;
		};

	const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
	if (Obj->TryGetArrayField(TEXT("HeavyFurniture"), Arr))
		Buckets.HeavyFurniture = DeserializeArray(*Arr);
	if (Obj->TryGetArrayField(TEXT("LightItems"), Arr))
		Buckets.LightItems = DeserializeArray(*Arr);
	if (Obj->TryGetArrayField(TEXT("Terminals"), Arr))
		Buckets.Terminals = DeserializeArray(*Arr);
	if (Obj->TryGetArrayField(TEXT("NPCSpawners"), Arr))
		Buckets.NPCSpawners = DeserializeArray(*Arr);
	if (Obj->TryGetArrayField(TEXT("Debris"), Arr))
		Buckets.Debris = DeserializeArray(*Arr);
	return Buckets;
}

// Serializes a population map to JSON / Сериализует карту населения в JSON
static TSharedPtr<FJsonValue> SerializePopulationMap(const TMap<FInteriorFloorKey, FFloorPopulationBuckets>& Map)
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	for (const auto& Pair : Map)
	{
		FString KeyStr = FString::Printf(TEXT("%s|%s"), *Pair.Key.InteriorSetId.ToString(), *Pair.Key.FloorId.ToString());
		Root->SetObjectField(KeyStr, PopulationBucketsToJson(Pair.Value));
	}
	return MakeShared<FJsonValueObject>(Root);
}

// Deserializes a population map from JSON / Десериализует карту населения из JSON
static void DeserializePopulationMap(const TSharedPtr<FJsonValue>& JsonValue, TMap<FInteriorFloorKey, FFloorPopulationBuckets>& OutMap)
{
	OutMap.Empty();
	if (!JsonValue.IsValid() || JsonValue->Type != EJson::Object) return;
	TSharedPtr<FJsonObject> Root = JsonValue->AsObject();
	for (const auto& Pair : Root->Values)
	{
		TArray<FString> Parts;
		Pair.Key.ParseIntoArray(Parts, TEXT("|"), false);
		if (Parts.Num() != 2) continue;
		FGuid InteriorSetId, FloorId;
		if (!FGuid::Parse(Parts[0], InteriorSetId) || !FGuid::Parse(Parts[1], FloorId)) continue;
		FInteriorFloorKey Key(InteriorSetId, FloorId);
		if (Pair.Value->Type == EJson::Object)
			OutMap.Add(Key, PopulationBucketsFromJson(Pair.Value->AsObject()));
	}
}

// Serializes a mission population map to JSON / Сериализует карту населения миссий в JSON
static TSharedPtr<FJsonValue> SerializeMissionPopulationMap(const TMap<FInteriorFloorKey, TMap<FName, FFloorPopulationBuckets>>& Map)
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	for (const auto& FloorPair : Map)
	{
		FString KeyStr = FString::Printf(TEXT("%s|%s"), *FloorPair.Key.InteriorSetId.ToString(), *FloorPair.Key.FloorId.ToString());
		TSharedPtr<FJsonObject> FloorObj = MakeShared<FJsonObject>();
		for (const auto& MissionPair : FloorPair.Value)
		{
			FloorObj->SetObjectField(MissionPair.Key.ToString(), PopulationBucketsToJson(MissionPair.Value));
		}
		Root->SetObjectField(KeyStr, FloorObj);
	}
	return MakeShared<FJsonValueObject>(Root);
}

// Deserializes a mission population map from JSON / Десериализует карту населения миссий из JSON
static void DeserializeMissionPopulationMap(const TSharedPtr<FJsonValue>& JsonValue,
	TMap<FInteriorFloorKey, TMap<FName, FFloorPopulationBuckets>>& OutMap)
{
	OutMap.Empty();
	if (!JsonValue.IsValid() || JsonValue->Type != EJson::Object) return;

	TSharedPtr<FJsonObject> Root = JsonValue->AsObject();
	for (const auto& Pair : Root->Values)
	{
		// Key format: "InteriorSetId|FloorId"
		// Ключ: "InteriorSetId|FloorId"
		TArray<FString> Parts;
		Pair.Key.ParseIntoArray(Parts, TEXT("|"), false);
		if (Parts.Num() != 2) continue;

		FGuid InteriorSetId, FloorId;
		if (!FGuid::Parse(Parts[0], InteriorSetId) || !FGuid::Parse(Parts[1], FloorId)) continue;
		FInteriorFloorKey FloorKey(InteriorSetId, FloorId);

		// Value: object where keys are mission names, values are bucket objects (FFloorPopulationBuckets)
		// Значение: объект, где ключи — имена миссий, значения — объекты бакетов (FFloorPopulationBuckets)
		if (Pair.Value->Type != EJson::Object) continue;
		TSharedPtr<FJsonObject> FloorObj = Pair.Value->AsObject();

		TMap<FName, FFloorPopulationBuckets> PerMissionMap;
		for (const auto& MissionPair : FloorObj->Values)
		{
			FName MissionId(*MissionPair.Key);
			if (MissionPair.Value->Type != EJson::Object) continue;

			// Deserialize FFloorPopulationBuckets from JSON object
			// Десериализуем FFloorPopulationBuckets из JSON-объекта
			FFloorPopulationBuckets Buckets = PopulationBucketsFromJson(MissionPair.Value->AsObject());
			PerMissionMap.Add(MissionId, Buckets);
		}
		OutMap.Add(FloorKey, PerMissionMap);
	}
}

auto HasChannelWithPolicy = [](const TArray<FEnvelopeChannelEntry>& Channels, EEnvelopeChannel Channel, EChannelPolicy Policy) -> bool
	{
		for (const FEnvelopeChannelEntry& Entry : Channels)
		{
			if (Entry.Channel == Channel && Entry.Policy == Policy)
				return true;
		}
		return false;
	};

// -----------------------------------------------------------------------------
// SaveFloorActorsState
// Сохранение состояния акторов этажа
// -----------------------------------------------------------------------------

void UInteriorSubsystem::SaveFloorActorsState(const FGuid& InteriorSetId, const FGuid& FloorId, FName MissionId, EMissionEndReason Reason/*, EJobSpacePolicy JobSpacePolicy, const TArray<FEnvelopeChannelEntry>& Channels*/)
{
	UWorld* W = GetWorld();
	if (!W) return;

	FInteriorFloorKey Key(InteriorSetId, FloorId);

	auto MissionInterior = ActiveMissions.Find(MissionId);
	FMissionEnvelope Envelope = FMissionEnvelope();
	TArray<FMissionEnvelope> Envelopes;
	int32 MissionStep;
	if (MissionInterior)
	{
		MissionStep = MissionInterior->MissionStep;
		auto Controller = MissionInterior->Controller;
		Envelopes = Controller->GetEnvelopes();
		if (Envelopes.IsValidIndex(MissionStep))
		{
			Envelope = Envelopes[MissionStep];
		}
	}
	else
	{
		return;
	}

	TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = MissionFloorSnapshots.FindOrAdd(Key);
	TArray<FFloorSavedActorState>& Bucket = PerFloor.FindOrAdd(MissionId);

	TMap<FGuid, int32> ExistingIndex;
	for (int32 i = 0; i < Bucket.Num(); ++i)
		ExistingIndex.Add(Bucket[i].ItemId, i);

	int32 AddedCount = 0;
	int32 UpdatedCount = 0;

	for (TActorIterator<AActor> It(W); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;

		UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
		if (!Comp) continue;
		if (Comp->SnapshotChannel == ESnapshotChannel::None) continue;

		const EEnvelopeChannel ActorChannel = FloorActorTypeToEnvelopeChannel(Comp->ActorType);

		if (Actor->IsChildActor()) continue;

		if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(Comp->ActorType), Reason, false))
		{
			switch (Comp->ActorType)
			{
			case EFloorActorType::SpawnGroupSpawner:
			{
				break;
			}

			case EFloorActorType::StableActor:
			{
				break;
			}
			}

			FFloorSavedActorState Snapshot;
			Snapshot.ItemId = Comp->ItemId;
			SnapshotActor(Actor, Snapshot);

			if (int32* FoundIdx = ExistingIndex.Find(Snapshot.ItemId))
			{
				Bucket[*FoundIdx] = MoveTemp(Snapshot);
				++UpdatedCount;
			}
			else
			{
				Bucket.Add(MoveTemp(Snapshot));
				ExistingIndex.Add(Bucket.Last().ItemId, Bucket.Num() - 1);
				++AddedCount;
			}
		}


		TArray<UChildActorComponent*> ChildComps;
		Actor->GetComponents<UChildActorComponent>(ChildComps);
		for (UChildActorComponent* ChildComp : ChildComps)
		{
			if (!IsValid(ChildComp)) continue;
			AActor* ChildActor = ChildComp->GetChildActor();
			if (!IsValid(ChildActor)) continue;

			UFloorAssignmentComponent* ChildFloorComp = ChildActor->FindComponentByClass<UFloorAssignmentComponent>();
			if (!ChildFloorComp) continue;
			if (ChildFloorComp->SnapshotChannel == ESnapshotChannel::None) continue;

			FFloorSavedActorState ChildSnapshot;
			ChildSnapshot.ItemId = ChildFloorComp->ItemId;
			ChildSnapshot.RelativeTransform = ChildComp->GetRelativeTransform();
			ChildSnapshot.bHasRelativeTransform = true;
			SnapshotActor(ChildActor, ChildSnapshot);

			if (int32* FoundIdx = ExistingIndex.Find(ChildSnapshot.ItemId))
			{
				Bucket[*FoundIdx] = MoveTemp(ChildSnapshot);
				++UpdatedCount;
			}
			else
			{
				Bucket.Add(MoveTemp(ChildSnapshot));
				ExistingIndex.Add(Bucket.Last().ItemId, Bucket.Num() - 1);
				++AddedCount;
			}
		}
	}

	if (!Envelopes.IsValidIndex(MissionStep))
	{
		CopyMissionSpawnedToGlobal(MissionSpawnedActorsByInteriorFloor, SpawnedActorsByInteriorFloor, Envelope, Reason, CurrentKey, MissionId);
		CopyMissionDestroyedToGlobal(MissionDestroyedActorsByInteriorFloor, DestroyedActorsByInteriorFloor, Envelope, Reason, CurrentKey, MissionId);
	}
}

void UInteriorSubsystem::SaveFloorActorsStateComplete(const FGuid& InteriorSetId, const FGuid& FloorId, FName MissionId, EMissionEndReason Reason/*, EJobSpacePolicy JobSpacePolicy, const TArray<FEnvelopeChannelEntry>& Channels*/)
{
	UWorld* W = GetWorld();
	if (!W) return;

	FInteriorFloorKey Key(InteriorSetId, FloorId);

	auto MissionInterior = ActiveMissions.Find(MissionId);
	FMissionEnvelope Envelope = FMissionEnvelope();
	TArray<FMissionEnvelope> Envelopes;
	int32 MissionStep;
	if (MissionInterior)
	{
		MissionStep = MissionInterior->MissionStep;
		auto Controller = MissionInterior->Controller;
		Envelopes = Controller->GetEnvelopes();
		if (Envelopes.IsValidIndex(MissionStep))
		{
			Envelope = Envelopes[MissionStep];
		}
	}
	else
	{
		return;
	}

	if (Reason != EMissionEndReason::None)
	{
		auto* PerFloor = MissionFloorSnapshots.Find(CurrentKey);
		if (PerFloor)
		{
			PerFloor->Remove(MissionId);
		}
	}

	// Existing logic: save to MissionFloorSnapshots[Key][MissionId]
	// существующая логика: сохраняем в MissionFloorSnapshots[Key][MissionId]
	TMap<FName, TArray<FFloorSavedActorState>>& PerFloorMission = MissionFloorSnapshots.FindOrAdd(Key);
	TArray<FFloorSavedActorState>& BucketMission = PerFloorMission.FindOrAdd(MissionId);

	TMap<FGuid, int32> ExistingIndex;
	for (int32 i = 0; i < BucketMission.Num(); ++i)
		ExistingIndex.Add(BucketMission[i].ItemId, i);

	TArray<FFloorSavedActorState>& BucketFloor = FloorStateSnapshots.FindOrAdd(Key);

	TMap<FGuid, int32> ExistingFloorIndex;
	for (int32 i = 0; i < BucketFloor.Num(); ++i)
		ExistingFloorIndex.Add(BucketFloor[i].ItemId, i);

	int32 AddedCount = 0;
	int32 UpdatedCount = 0;

	for (TActorIterator<AActor> It(W); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;

		UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
		if (!Comp) continue;
		if (Comp->SnapshotChannel == ESnapshotChannel::None) continue;

		if (Actor->IsChildActor()) continue;

		const EEnvelopeChannel ActorChannel = FloorActorTypeToEnvelopeChannel(Comp->ActorType);

		EJobSpacePolicy CurrentPolicy = EJobSpacePolicy::None;
		TArray<FEnvelopeChannelEntry> CurrentPolicyChannels;

		switch (Reason)
		{
			case EMissionEndReason::None:
			{
				//CurrentPolicy = Envelope.RuntimePolicy;
				//CurrentPolicyChannels = Envelope.RuntimePolicyChannels;
				break;
			}
			case EMissionEndReason::Completed:
			{
				CurrentPolicy = Envelope.NextStagePolicy;
				CurrentPolicyChannels = Envelope.NextStagePolicyChannels;
				break;
			}
			case EMissionEndReason::Failed:
			{
				CurrentPolicy = Envelope.MissionFailedPolicy;
				CurrentPolicyChannels = Envelope.MissionFailedPolicyChannels;
				break;
			}
			case EMissionEndReason::Abandoned:
			{
				CurrentPolicy = Envelope.MissionAbandonedPolicy;
				CurrentPolicyChannels = Envelope.MissionAbandonedChannels;
				break;
			}
		}

		switch (Comp->ActorType)
		{
			case EFloorActorType::SpawnGroupSpawner :
			{
				if (!Envelopes.IsValidIndex(MissionStep + 1) || Reason != EMissionEndReason::Completed)
				{
					ASpawnGroupSpawner* Spawner = Cast<ASpawnGroupSpawner>(Actor);
					if (Spawner)
					{
						Spawner->StoreSpawnParameters();

						switch (CurrentPolicy)
						{
							case EJobSpacePolicy::Reset:
							{
								// в этом случае данные не будут сохранены
								Spawner->IsUseStoreSpawnParameters = false;
								break;
							}
							case EJobSpacePolicy::Freeze:
							{
								Spawner->CurrentStatus = ESpawnGroupStatus::Suppressed;
								Spawner->ResetKilledCount();
								Spawner->IsUseStoreSpawnParameters = true;
								break;
							}
							case EJobSpacePolicy::Partial:
							{
								if (!HasChannelWithPolicy(CurrentPolicyChannels, EEnvelopeChannel::SpawnGroups, EChannelPolicy::Reset))
								{
									Spawner->CurrentStatus = ESpawnGroupStatus::Suppressed;
									Spawner->ResetKilledCount();
									Spawner->IsUseStoreSpawnParameters = false;
								}
								break;
							}
						}
					}
				}
				else
				{
					FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(W, true);
					FMissionEnvelopeScope Scope = Envelopes[MissionStep + 1].Scope;
					bool NextStageIsCurrentFloor = false;
					for (auto Sc : Scope.InteriorScopes)
					{
						FString ScopeLevelName = Sc->FloorLevel.ToSoftObjectPath().GetLongPackageName();

						FString NormTarget = NormalizeLevelName(ScopeLevelName);
						FString NormCurrent = NormalizeLevelName(CurrentLevelName);

						if (NormTarget == NormCurrent)
						{
							NextStageIsCurrentFloor = true;
							break;
						}
					}
					if (!NextStageIsCurrentFloor)
					{
						ASpawnGroupSpawner* Spawner = Cast<ASpawnGroupSpawner>(Actor);
						if (Spawner)
						{
							switch (CurrentPolicy)
							{
								case EJobSpacePolicy::Reset:
								{
									// в этом случае данные не будут сохранены
									Spawner->IsUseStoreSpawnParameters = false;
									break;
								}
								case EJobSpacePolicy::Freeze:
								{
									Spawner->CurrentStatus = ESpawnGroupStatus::Suppressed;
									Spawner->ResetKilledCount();
									Spawner->IsUseStoreSpawnParameters = true;
									break;
								}
								case EJobSpacePolicy::Partial:
								{
									if (!HasChannelWithPolicy(CurrentPolicyChannels, EEnvelopeChannel::SpawnGroups, EChannelPolicy::Reset))
									{
										Spawner->CurrentStatus = ESpawnGroupStatus::Suppressed;
										Spawner->ResetKilledCount();
										Spawner->IsUseStoreSpawnParameters = false;
									}
									break;
								}
							}
						}
					}
					else
					{
						if (Reason == EMissionEndReason::Completed)
						{
							switch (CurrentPolicy)
							{
								case EJobSpacePolicy::Reset:
								{
									ASpawnGroupSpawner* Spawner = Cast<ASpawnGroupSpawner>(Actor);
									if (Spawner)
									{
										Spawner->ResetKilledCount();
										Spawner->CurrentStatus = ESpawnGroupStatus::Inactive;
										Spawner->BlockNewSpawn = false;
										Spawner->IsUseStoreSpawnParameters = true;
									}
									break;
								}
								case EJobSpacePolicy::Freeze:
								{
									ASpawnGroupSpawner* Spawner = Cast<ASpawnGroupSpawner>(Actor);
									if (Spawner)
									{
										Spawner->CurrentStatus = ESpawnGroupStatus::Suppressed;
										Spawner->BlockNewSpawn = true;
										Spawner->IsUseStoreSpawnParameters = true;
									}
									break;
								}
								case EJobSpacePolicy::Partial:
								{
									if (HasChannelWithPolicy(CurrentPolicyChannels, EEnvelopeChannel::SpawnGroups, EChannelPolicy::ResetUnlessCleared))
									{
										ASpawnGroupSpawner* Spawner = Cast<ASpawnGroupSpawner>(Actor);
										if (Spawner)
										{
											Spawner->BlockNewSpawn = true;
											Spawner->IsUseStoreSpawnParameters = false;

											if (Spawner->GetSpawnedCount() == Spawner->GetKilledCount())
											{
												Spawner->ResetKilledCount();
												Spawner->CurrentStatus = ESpawnGroupStatus::Suppressed;
											}
											else
											{
												/*
												Spawner->ResetKilledCount();
												Spawner->CurrentStatus = ESpawnGroupStatus::Active;
												*/
												Spawner->ResetKilledCount();
												Spawner->CurrentStatus = ESpawnGroupStatus::PartiallyCleared;
												Spawner->Restore = true;
											}
										}
									}
									else
									{
										if (HasChannelWithPolicy(CurrentPolicyChannels, EEnvelopeChannel::SpawnGroups, EChannelPolicy::Reset))
										{
											ASpawnGroupSpawner* Spawner = Cast<ASpawnGroupSpawner>(Actor);
											if (Spawner)
											{
												Spawner->ResetKilledCount();
												Spawner->CurrentStatus = ESpawnGroupStatus::Inactive;
												Spawner->BlockNewSpawn = false;
												Spawner->IsUseStoreSpawnParameters = false;
											}
										}
										else
										{
											ASpawnGroupSpawner* Spawner = Cast<ASpawnGroupSpawner>(Actor);
											if (Spawner)
											{
												Spawner->BlockNewSpawn = false;
											}
										}
									}
									break;
								}
							}
						}
					}
				}

				break;
			}
			case EFloorActorType::StableActor:
			{
				// будет реализовано позже
				break;
			}
		}

		FFloorSavedActorState Snapshot;
		Snapshot.ItemId = Comp->ItemId;
		SnapshotActor(Actor, Snapshot);

		if (int32* FoundIdx = ExistingIndex.Find(Snapshot.ItemId))
		{
			BucketMission[*FoundIdx] = Snapshot;
			++UpdatedCount;
		}
		else
		{
			BucketMission.Add(Snapshot);
			ExistingIndex.Add(BucketMission.Last().ItemId, BucketMission.Num() - 1);
			++AddedCount;
		}

		if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(Comp->ActorType), Reason, false))
		{
			if (int32* FoundFloorIdx = ExistingFloorIndex.Find(Snapshot.ItemId))
			{
				BucketFloor[*FoundFloorIdx] = Snapshot;

			}
			else
			{
				BucketFloor.Add(Snapshot);
				ExistingFloorIndex.Add(BucketFloor.Last().ItemId, BucketFloor.Num() - 1);
			}
		}

		TArray<UChildActorComponent*> ChildComps;
		Actor->GetComponents<UChildActorComponent>(ChildComps);
		for (UChildActorComponent* ChildComp : ChildComps)
		{
			if (!IsValid(ChildComp)) continue;
			AActor* ChildActor = ChildComp->GetChildActor();

			if (!IsValid(ChildActor)) continue;

			UFloorAssignmentComponent* ChildFloorComp = ChildActor->FindComponentByClass<UFloorAssignmentComponent>();
			if (!ChildFloorComp) continue;
			if (ChildFloorComp->SnapshotChannel == ESnapshotChannel::None) continue;

			FFloorSavedActorState ChildSnapshot;
			ChildSnapshot.ItemId = ChildFloorComp->ItemId;
			ChildSnapshot.RelativeTransform = ChildComp->GetRelativeTransform();
			ChildSnapshot.bHasRelativeTransform = true;
			SnapshotActor(ChildActor, ChildSnapshot);

			if (int32* ChildIdx = ExistingIndex.Find(ChildSnapshot.ItemId))
			{
				BucketMission[*ChildIdx] = ChildSnapshot;
				++UpdatedCount;
			}
			else
			{
				BucketMission.Add(ChildSnapshot);
				ExistingIndex.Add(BucketMission.Last().ItemId, BucketMission.Num() - 1);
				++AddedCount;
			}


			if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(ChildFloorComp->ActorType), Reason, false))
			{
				if (int32* FoundFloorIdx = ExistingFloorIndex.Find(ChildSnapshot.ItemId))
				{
					BucketFloor[*FoundFloorIdx] = ChildSnapshot;

				}
				else
				{
					BucketFloor.Add(ChildSnapshot);
					ExistingFloorIndex.Add(BucketFloor.Last().ItemId, BucketFloor.Num() - 1);
				}

			}
		}
	}

	CopyMissionSpawnedToGlobal(MissionSpawnedActorsByInteriorFloor, SpawnedActorsByInteriorFloor, Envelope, Reason, CurrentKey, MissionId);
	CopyMissionDestroyedToGlobal(MissionDestroyedActorsByInteriorFloor, DestroyedActorsByInteriorFloor, Envelope, Reason, CurrentKey, MissionId);
}


// -----------------------------------------------------------------------------
// RestoreFloorActorsState
// Восстановление состояния акторов этажа
// -----------------------------------------------------------------------------

int32 UInteriorSubsystem::RestoreFloorActorsState(const FGuid& InteriorSetId, int32 CurrentMissionStep, const FGuid& FloorId)
{
	UWorld* W = GetWorld();
	if (!W) return 0;

	FInteriorFloorKey Key(InteriorSetId, FloorId);



	// Collect floor actors into a dictionary by ItemId
	// Собираем актёров этажа в словарь по ItemId
	TMap<FGuid, AActor*> ActorByItemId;
	for (TActorIterator<AActor> It(W); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;
		if (UFloorAssignmentComponent* C = Actor->FindComponentByClass<UFloorAssignmentComponent>())
		{
			if (C->SnapshotChannel != ESnapshotChannel::None)
				ActorByItemId.Add(C->ItemId, Actor);
		}

		TArray<UChildActorComponent*> ChildComps;
		Actor->GetComponents<UChildActorComponent>(ChildComps);
		for (UChildActorComponent* ChildComp : ChildComps)
		{
			if (!IsValid(ChildComp)) continue;
			AActor* ChildActor = ChildComp->GetChildActor();
			if (!IsValid(ChildActor)) continue;
			if (UFloorAssignmentComponent* CC = ChildActor->FindComponentByClass<UFloorAssignmentComponent>())
			{
				if (CC->SnapshotChannel != ESnapshotChannel::None)
					ActorByItemId.Add(CC->ItemId, ChildActor);
			}
		}
	}

	// Find all mission snapshots for this floor
	// Ищем все снимки миссий для данного этажа
	const TMap<FName, TArray<FFloorSavedActorState>>* PerFloor = MissionFloorSnapshots.Find(Key);
	if (!PerFloor || PerFloor->Num() == 0) return 0;

	int32 TotalRestored = 0;

	// Restore snapshots for each mission attached to this floor
	// Восстанавливаем снимки для каждой миссии, привязанной к этому этажу
	for (const auto& MissionPair : *PerFloor)
	{
		const TArray<FFloorSavedActorState>& Snapshots = MissionPair.Value;

		// First pass: independent (root) actors
		// First pass: независимые (root) акторы
		for (const FFloorSavedActorState& Snapshot : Snapshots)
		{
			if (Snapshot.bHasRelativeTransform) continue;
			AActor** ActorPtr = ActorByItemId.Find(Snapshot.ItemId);
			if (!ActorPtr || !IsValid(*ActorPtr)) continue;
			RestoreActorSnapshot(*ActorPtr, Snapshot, true, true);
			++TotalRestored;
		}

		// Second pass: child actors
		// Second pass: дочерние (child) акторы
		for (const FFloorSavedActorState& Snapshot : Snapshots)
		{
			if (!Snapshot.bHasRelativeTransform) continue;
			AActor** ActorPtr = ActorByItemId.Find(Snapshot.ItemId);
			if (!ActorPtr || !IsValid(*ActorPtr)) continue;
			AActor* ChildActor = *ActorPtr;

			AActor* ParentActor = ChildActor->GetAttachParentActor();
			bool bAppliedRelative = false;
			if (IsValid(ParentActor))
			{
				TArray<UChildActorComponent*> ParentChildComps;
				ParentActor->GetComponents<UChildActorComponent>(ParentChildComps);
				for (UChildActorComponent* ParentChildComp : ParentChildComps)
				{
					if (!IsValid(ParentChildComp)) continue;
					if (ParentChildComp->GetChildActor() == ChildActor)
					{
						ParentChildComp->SetRelativeTransform(Snapshot.RelativeTransform);
						RestoreActorSnapshot(ChildActor, Snapshot, false, true);
						++TotalRestored;
						bAppliedRelative = true;
						break;
					}
				}
			}

			if (!bAppliedRelative)
			{
				ChildActor->SetActorTransform(Snapshot.ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);

				RestoreActorSnapshot(ChildActor, Snapshot, false, true);
				++TotalRestored;
			}

		}
	}

	return TotalRestored;
}

// --- New helper: restores actors directly from a given snapshot array
// --- добавлен новый хелпер: восстанавливает actors напрямую из переданного массива снимков
int32 UInteriorSubsystem::RestoreFromSnapshotArray(UWorld* W, const TArray<FFloorSavedActorState>& Snapshots, const FGuid& InteriorSetId, const FGuid& FloorId, const FMissionEnvelope Envelope)
{
	if (!W)
	{
		return 0;
	}

	if (Snapshots.IsEmpty())
	{
		return 0;
	}

	// Collect all actors on the current level that have FloorAssignmentComponent
	// Собираем всех акторов текущего уровня, имеющих FloorAssignmentComponent
	TMap<FGuid, AActor*> ActorByItemId;
	for (TActorIterator<AActor> It(W); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;
		if (UFloorAssignmentComponent* C = Actor->FindComponentByClass<UFloorAssignmentComponent>())
		{
			if (C->SnapshotChannel != ESnapshotChannel::None)
			{
				if (ContainsActorIdInSpawnedNew(DestroyedActorsByInteriorFloor, C->ItemId))
				{
					Actor->Destroy();
					continue;
				}
				ActorByItemId.Add(C->ItemId, Actor);
			}
		}

		TArray<UChildActorComponent*> ChildComps;
		Actor->GetComponents<UChildActorComponent>(ChildComps);
		for (UChildActorComponent* ChildComp : ChildComps)
		{
			if (!IsValid(ChildComp)) continue;
			AActor* ChildActor = ChildComp->GetChildActor();
			if (!IsValid(ChildActor)) continue;
			if (UFloorAssignmentComponent* CC = ChildActor->FindComponentByClass<UFloorAssignmentComponent>())
			{
				if (CC->SnapshotChannel != ESnapshotChannel::None)
					ActorByItemId.Add(CC->ItemId, ChildActor);
			}
		}
	}

	int32 Restored = 0;

	// First pass: root actors (without relative transform)
	// First pass: root actors (без относительного преобразования)
	for (const FFloorSavedActorState& Snapshot : Snapshots)
	{
		if (Snapshot.bHasRelativeTransform) continue;
		if (AActor** ActorPtr = ActorByItemId.Find(Snapshot.ItemId))
		{
			AActor* Actor = *ActorPtr;
			if (IsValid(Actor))
			{
				if (UFloorAssignmentComponent* C = Actor->FindComponentByClass<UFloorAssignmentComponent>())
				{
					if (Envelope.IsValid())
					{
						if (IsCurrentWorldMissionConst(Envelope))
						{
							if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(C->ActorType), EMissionEndReason::None, true))
							{
								RestoreActorSnapshot(Actor, Snapshot, true);
								++Restored;

								switch (FloorActorTypeToEnvelopeChannel(C->ActorType))
								{
									case EEnvelopeChannel::SpawnGroups:
									{
										switch (Envelope.RuntimePolicy)
										{
										case EJobSpacePolicy::Freeze:
										{
											ASpawnGroupSpawner* Spawner = Cast<ASpawnGroupSpawner>(Actor);
											if (Spawner)
											{
												Spawner->IsUseStoreSpawnParameters = true;
												//Spawner->Restore = false;
											}

											break;
										}
										case EJobSpacePolicy::Reset:
										{
											ASpawnGroupSpawner* Spawner = Cast<ASpawnGroupSpawner>(Actor);
											if (Spawner)
											{
												Spawner->IsUseStoreSpawnParameters = false;
												Spawner->Restore = false;
											}

											break;
										}
										case EJobSpacePolicy::Partial:
										{
											if (HasChannelWithPolicy(Envelope.RuntimePolicyChannels, EEnvelopeChannel::SpawnGroups, EChannelPolicy::ResetUnlessCleared))
											{
												ASpawnGroupSpawner* Spawner = Cast<ASpawnGroupSpawner>(Actor);
												if (Spawner)
												{
													Spawner->IsUseStoreSpawnParameters = false;
													if (Spawner->CurrentStatus != ESpawnGroupStatus::Cleared)
													{
														Spawner->ResetKilledCount();
														Spawner->Restore = true;
													}
												}
											}
											break;
										}
										}
										break;
									}
									case EEnvelopeChannel::ActorPlacement:
									{
										break;
									}
								}
							}
						}
					}
					else
					{
						RestoreActorSnapshot(Actor, Snapshot, true);
						++Restored;
					}
				}

			}
		}
	}

	// Second pass: child actors (with relative transform)
	// Second pass: child actors (с относительным преобразованием)
	for (const FFloorSavedActorState& Snapshot : Snapshots)
	{
		if (!Snapshot.bHasRelativeTransform) continue;
		if (AActor** ActorPtr = ActorByItemId.Find(Snapshot.ItemId))
		{
			AActor* ChildActor = *ActorPtr;
			if (!IsValid(ChildActor)) continue;
			if (UFloorAssignmentComponent* CC = ChildActor->FindComponentByClass<UFloorAssignmentComponent>())
			{
				AActor* ParentActor = ChildActor->GetAttachParentActor();
				bool bAppliedRelative = false;
				if (IsValid(ParentActor))
				{
					TArray<UChildActorComponent*> ParentChildComps;
					ParentActor->GetComponents<UChildActorComponent>(ParentChildComps);
					for (UChildActorComponent* ParentChildComp : ParentChildComps)
					{
						if (IsValid(ParentChildComp) && ParentChildComp->GetChildActor() == ChildActor)
						{
							if (Envelope.IsValid())
							{
								if (IsCurrentWorldMissionConst(Envelope))
								{
									if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(CC->ActorType), EMissionEndReason::None, true))
									{
										ParentChildComp->SetRelativeTransform(Snapshot.RelativeTransform);
										RestoreActorSnapshot(ChildActor, Snapshot, false, true);

										bAppliedRelative = true;
										break;
									}
								}
							}
							else
							{
								ParentChildComp->SetRelativeTransform(Snapshot.RelativeTransform);
								RestoreActorSnapshot(ChildActor, Snapshot, false, true);

								bAppliedRelative = true;
								break;
							}
						}
					}
				}
				if (Envelope.IsValid())
				{
					if (IsCurrentWorldMissionConst(Envelope))
					{
						if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(CC->ActorType), EMissionEndReason::None, true))
						{
							if (!bAppliedRelative)
								ChildActor->SetActorTransform(Snapshot.ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);
							RestoreActorSnapshot(ChildActor, Snapshot, false, true);
						}
					}
				}
				else
				{
					if (!bAppliedRelative)
						ChildActor->SetActorTransform(Snapshot.ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);
					RestoreActorSnapshot(ChildActor, Snapshot, false, true);
				}
			}
		}
	}

	return Restored;
}

// -----------------------------------------------------------------------------
// EventBus handlers — Registration
// Обработчики EventBus — регистрация
// -----------------------------------------------------------------------------

void UInteriorSubsystem::HandleInteractRegistration(const FOutcomeEventBase& Outcome)
{
	UInteractItemRegistrationPayload* P = Cast<UInteractItemRegistrationPayload>(Outcome.Payload);
	if (!P) return;

	AActor* Owner = P->GetOwnerActor();
	const FString OwnerName = Owner ? Owner->GetName() : FString(TEXT("Unknown"));

	if (Outcome.OutcomeInterior == EOutcomeInterior::InteractRegistered)
	{
		FInteractItemRecord R;
		R.ItemId = P->ItemId;
		R.OwnerActor = Owner;
		R.InteractionRange = P->InteractionRange;
		R.SubsystemType = P->SubsystemType;
		RegisteredItems.Add(P->ItemId, R);

		if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(P->ItemId))
		{
			for (auto It = List->CreateIterator(); It; ++It)
			{
				if (UInteractiveItemComponent* Comp = It->Get())
					Comp->OnRegisteredBySubsystem(P);
			}
			RegistrationListeners.Remove(P->ItemId);
		}

		OnInteractItemRegistered.Broadcast(P);
	}
	else if (Outcome.OutcomeInterior == EOutcomeInterior::InteractUnregistered)
	{
		RegisteredItems.Remove(P->ItemId);

		if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(P->ItemId))
		{
			for (auto It = List->CreateIterator(); It; ++It)
			{
				if (UInteractiveItemComponent* Comp = It->Get())
					Comp->OnUnregisteredBySubsystem(P);
			}
			RegistrationListeners.Remove(P->ItemId);
		}

		OnInteractItemUnregistered.Broadcast(P);
	}
}

void UInteriorSubsystem::HandlePlacementRegistration(const FOutcomeEventBase& Outcome)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UFloorPlacementPayload* P = Cast<UFloorPlacementPayload>(Outcome.Payload);
	if (!P) return;

	if (!P->ItemId.IsValid())
	{
		return;
	}

	// Use the current floor (CurrentKey) where spawn/destroy happened
	// (in your code earlier CurrentKey was used, not the passed P->InteriorSetId/P->FloorId)
	// Используем текущий этаж (CurrentKey), на котором произошёл спавн/удаление
	// (в вашем коде ранее использовался CurrentKey, а не переданный P->InteriorSetId/P->FloorId)
	if (Outcome.OutcomeInterior == EOutcomeInterior::FloorPlacementRegistered)
	{
		bool IsMissionWorld = false;

		FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
		// FString MissionName;
		FName MissionId;
		for (const auto& Pair : ActiveMissions)
		{
			const UMissionController* Ctrl = Pair.Value.Controller;
			if (!Ctrl) continue;

			const UMissionAsset* Asset = Ctrl->GetMissionAsset();
			if (!Asset) continue;

			// MissionName = Asset->Description.ToString();
			const EMissionStatus Status = Ctrl->GetStatus();
			const EMissionEndReason EndReason = Ctrl->GetEndReason();
			const FMissionEnvelope Envelope = Asset->Envelopes[Pair.Value.MissionStep];
			const EMissionResumeMode Resume = Envelope.ResumeMode;
			const int32 MissionStep = Pair.Value.MissionStep;
			MissionId = Pair.Key;

			FMissionEnvelopeScope Scope = Envelope.Scope;
			for (auto Sc : Scope.InteriorScopes)
			{
				FString ScopeLevelName = Sc->FloorLevel.ToSoftObjectPath().GetLongPackageName();

				FString NormTarget = NormalizeLevelName(ScopeLevelName);
				FString NormCurrent = NormalizeLevelName(CurrentLevelName);

				if (NormTarget == NormCurrent)
				{
					IsMissionWorld = true;
					break;
				}
			}

			if (IsMissionWorld)
			{
				break;
			}
		}


		// Add record to SpawnedActorsByInteriorFloor
		// Добавляем запись в SpawnedActorsByInteriorFloor
		FFloorPopulationRecord NewRecord;
		NewRecord.ActorType = P->ActorType;
		NewRecord.WorldTransform = P->WorldTransform;
		NewRecord.ActorId = P->ItemId;
		NewRecord.bHasAnchor = P->AnchorId.IsValid();
		NewRecord.SourceClass = P->ActorClass;

		if (IsMissionWorld)
		{
			if (MissionId.IsNone()) return;

			TMap<FName, FFloorPopulationBuckets>& PerMissionSpawn = MissionSpawnedActorsByInteriorFloor.FindOrAdd(CurrentKey);
			FFloorPopulationBuckets& MissionBuckets = PerMissionSpawn.FindOrAdd(MissionId);
			switch (P->ActorType)
			{
			case EFloorActorType::HeavyFurniture:
				if (!MissionBuckets.HeavyFurniture.ContainsByPredicate([&NewRecord](const FFloorPopulationRecord& Existing) { return Existing.ActorId == NewRecord.ActorId; }))
					MissionBuckets.HeavyFurniture.Add(NewRecord);
				break;
			case EFloorActorType::LightItem:
				if (!MissionBuckets.LightItems.ContainsByPredicate([&NewRecord](const FFloorPopulationRecord& Existing) { return Existing.ActorId == NewRecord.ActorId; }))
					MissionBuckets.LightItems.Add(NewRecord);
				break;
			case EFloorActorType::Terminal:
				if (!MissionBuckets.Terminals.ContainsByPredicate([&NewRecord](const FFloorPopulationRecord& Existing) { return Existing.ActorId == NewRecord.ActorId; }))
					MissionBuckets.Terminals.Add(NewRecord);
				break;
			case EFloorActorType::SpawnGroupSpawner:
				if (!MissionBuckets.NPCSpawners.ContainsByPredicate([&NewRecord](const FFloorPopulationRecord& Existing) { return Existing.ActorId == NewRecord.ActorId; }))
					MissionBuckets.NPCSpawners.Add(NewRecord);
				break;
			case EFloorActorType::Debris:
				if (!MissionBuckets.Debris.ContainsByPredicate([&NewRecord](const FFloorPopulationRecord& Existing) { return Existing.ActorId == NewRecord.ActorId; }))
					MissionBuckets.Debris.Add(NewRecord);
				break;
			default:
				break;
			}
		}
	}
	else if (Outcome.OutcomeInterior == EOutcomeInterior::FloorPlacementUnregistered)
	{
		if (!World) return;

		bool IsMissionWorld = false;
		FName MissionId;

		FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
		for (const auto& Pair : ActiveMissions)
		{
			const UMissionController* Ctrl = Pair.Value.Controller;
			if (!Ctrl) continue;

			const UMissionAsset* Asset = Ctrl->GetMissionAsset();
			if (!Asset) continue;

			const EMissionStatus Status = Ctrl->GetStatus();
			const EMissionEndReason EndReason = Ctrl->GetEndReason();
			const FMissionEnvelope Envelope = Asset->Envelopes[Pair.Value.MissionStep];
			const EMissionResumeMode Resume = Envelope.ResumeMode;
			const int32 MissionStep = Pair.Value.MissionStep;

			FMissionEnvelopeScope Scope = Envelope.Scope;
			for (auto Sc : Scope.InteriorScopes)
			{
				FString ScopeLevelName = Sc->FloorLevel.ToSoftObjectPath().GetLongPackageName();

				FString NormTarget = NormalizeLevelName(ScopeLevelName);
				FString NormCurrent = NormalizeLevelName(CurrentLevelName);

				if (NormTarget == NormCurrent)
				{
					IsMissionWorld = true;
					MissionId = Pair.Key;
					break;
				}
			}

			if (IsMissionWorld)
			{
				break;
			}
		}

		if (MissionId.IsNone()) return;

		TMap<FName, FFloorPopulationBuckets>& PerMissionDestroy = MissionDestroyedActorsByInteriorFloor.FindOrAdd(CurrentKey);
		FFloorPopulationBuckets& MissionBuckets = PerMissionDestroy.FindOrAdd(MissionId);

		// FFloorPopulationBuckets& BucketsDestroyed = DestroyedActorsByInteriorFloor.FindOrAdd(CurrentKey);
		FFloorPopulationRecord NewRecordMission;
		NewRecordMission.ActorType = P->ActorType;
		NewRecordMission.WorldTransform = P->WorldTransform;
		NewRecordMission.ActorId = P->ItemId;
		NewRecordMission.bHasAnchor = P->AnchorId.IsValid();

		switch (P->ActorType)
		{
		case EFloorActorType::HeavyFurniture:
		{
			if (!MissionBuckets.HeavyFurniture.ContainsByPredicate([&NewRecordMission](const FFloorPopulationRecord& Existing) {
				return Existing.ActorId == NewRecordMission.ActorId;
				}))
			{
				MissionBuckets.HeavyFurniture.Add(NewRecordMission);
			}
			break;
		}

		case EFloorActorType::LightItem:
		{
			if (!MissionBuckets.LightItems.ContainsByPredicate([&NewRecordMission](const FFloorPopulationRecord& Existing) {
				return Existing.ActorId == NewRecordMission.ActorId;
				}))
			{
				MissionBuckets.LightItems.Add(NewRecordMission);
			}
			break;
		}

		case EFloorActorType::Terminal:
		{
			if (!MissionBuckets.Terminals.ContainsByPredicate([&NewRecordMission](const FFloorPopulationRecord& Existing) {
				return Existing.ActorId == NewRecordMission.ActorId;
				}))
			{
				MissionBuckets.Terminals.Add(NewRecordMission);
			}
			break;
		}

		case EFloorActorType::SpawnGroupSpawner:
		{
			if (!MissionBuckets.NPCSpawners.ContainsByPredicate([&NewRecordMission](const FFloorPopulationRecord& Existing) {
				return Existing.ActorId == NewRecordMission.ActorId;
				}))
			{
				MissionBuckets.NPCSpawners.Add(NewRecordMission);
			}
			break;
		}

		case EFloorActorType::Debris:
		{
			if (!MissionBuckets.Debris.ContainsByPredicate([&NewRecordMission](const FFloorPopulationRecord& Existing) {
				return Existing.ActorId == NewRecordMission.ActorId;
				}))
			{
				MissionBuckets.Debris.Add(NewRecordMission);
			}
			break;
		}

		default: {}
		}

		// Remove record from SpawnedActorsByInteriorFloor
		// Удаляем запись из SpawnedActorsByInteriorFloor
		auto RemoveFromSpawnedByActorId = [](TMap<FInteriorFloorKey, FFloorPopulationBuckets>& SpawnedMap, const FGuid& TargetActorId)
			{
				if (!TargetActorId.IsValid()) return;
				for (auto& Pair : SpawnedMap)
				{
					FFloorPopulationBuckets& Buckets = Pair.Value;
					auto RemoveByActorId = [&TargetActorId](TArray<FFloorPopulationRecord>& Arr)
						{
							Arr.RemoveAll([&TargetActorId](const FFloorPopulationRecord& Record)
								{
									return Record.ActorId == TargetActorId;
								});
						};
					RemoveByActorId(Buckets.HeavyFurniture);
					RemoveByActorId(Buckets.LightItems);
					RemoveByActorId(Buckets.Terminals);
					RemoveByActorId(Buckets.NPCSpawners);
					RemoveByActorId(Buckets.Debris);
				}
			};

		if (!MissionId.IsNone())
		{
			if (ContainsActorIdInMissionSpawnedForFloor(MissionSpawnedActorsByInteriorFloor, CurrentKey, MissionId, P->ItemId))
			{
				RemoveFromMissionDestroyedByActorIdForFloor(MissionDestroyedActorsByInteriorFloor, CurrentKey, MissionId, P->ItemId);
			}
			RemoveFromMissionSpawnedByActorIdForFloor(MissionSpawnedActorsByInteriorFloor, CurrentKey, MissionId, P->ItemId);
		}
	}
}

bool UInteriorSubsystem::IsCurrentWorldMissionConst(const FMissionEnvelope& Envelope)
{
	UWorld* World = GetWorld();
	if (!World) return false;

	bool IsMissionWorld = false;

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);

	FMissionEnvelopeScope Scope = Envelope.Scope;
	for (auto Sc : Scope.InteriorScopes)
	{
		FString ScopeLevelName = Sc->FloorLevel.ToSoftObjectPath().GetLongPackageName();

		FString NormTarget = NormalizeLevelName(ScopeLevelName);
		FString NormCurrent = NormalizeLevelName(CurrentLevelName);

		if (NormTarget == NormCurrent)
		{
			IsMissionWorld = true;
			break;
		}
	}

	return IsMissionWorld;
}

// -----------------------------------------------------------------------------
// EventBus handlers — Interact commands
// Обработчики EventBus — команды взаимодействия
// -----------------------------------------------------------------------------

void UInteriorSubsystem::HandleInteractCommand(const FOutcomeEventBase& Outcome)
{
	const UInteractCommandPayload* P = Cast<UInteractCommandPayload>(Outcome.Payload);
	if (!P) return;
	if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
	{
		if (AActor* Owner = Rec->OwnerActor.Get())
			ExecuteInteractCommandOnOwner(P->ItemId, Owner, P->Picker);
	}
}

void UInteriorSubsystem::HandleSetEnabled(const FOutcomeEventBase& Outcome)
{
	const UInteractSetEnabledPayload* P = Cast<UInteractSetEnabledPayload>(Outcome.Payload);
	if (!P) return;
	if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		ExecuteSetEnabledOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->bEnabled);
}

void UInteriorSubsystem::HandleSetRange(const FOutcomeEventBase& Outcome)
{
	const UInteractSetRangePayload* P = Cast<UInteractSetRangePayload>(Outcome.Payload);
	if (!P) return;
	if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		ExecuteSetRangeOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewRange);
}

void UInteriorSubsystem::HandleSetTooltip(const FOutcomeEventBase& Outcome)
{
	const UInteractSetTooltipPayload* P = Cast<UInteractSetTooltipPayload>(Outcome.Payload);
	if (!P) return;
	if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		ExecuteSetTooltipOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewTooltip);
}

// -----------------------------------------------------------------------------
// EventBus handlers — Floor snapshot
// Обработчики EventBus — снимки этажа
// -----------------------------------------------------------------------------

void UInteriorSubsystem::HandleFloorStateSave(const FOutcomeEventBase& Outcome)
{
	if (UFloorStatePayload* P = Cast<UFloorStatePayload>(Outcome.Payload))
		SaveFloorActorsState(P->InteriorSetId, P->FloorId, P->MissionId, EMissionEndReason::None);
}

void UInteriorSubsystem::HandleFloorStateRestore(const FOutcomeEventBase& Outcome)
{
	if (UFloorStatePayload* P = Cast<UFloorStatePayload>(Outcome.Payload))
	{
		int32 CurrentMissionStep = P->CurrentMissionStep;
		if (CurrentMissionStep < 0)
		{
			return;
		}

		if (P->MissionId != NAME_None)
		{
			RestoreMissionFloorState(P->MissionId, CurrentMissionStep, FInteriorFloorKey(P->InteriorSetId, P->FloorId));
		}
		else
		{
			RestoreFloorActorsState(P->InteriorSetId, CurrentMissionStep, P->FloorId);
		}
	}
}

// -----------------------------------------------------------------------------
// HandleFloorTransition
// Обработка перехода между этажами
// -----------------------------------------------------------------------------
void UInteriorSubsystem::HandleFloorTransition(const FOutcomeEventBase& Outcome)
{
	UWorld* World = GetWorld();
	if (!World) return;

	UInteriorTransitionPayload* P = Cast<UInteriorTransitionPayload>(Outcome.Payload);
	if (!P || (P->DestinationLink.IsValid() && !P->IsUseAnchor) || (!P->DestinationLink.IsValid() && P->IsUseAnchor))
	{
		OnTransitionCompleted.Broadcast(false, FLocationAnchorLink(), false);
		return;
	}

	TransitionPayloadCache = P;
	UWorld* W = GetWorld();
	if (!W)
	{
		OnTransitionCompleted.Broadcast(false, FLocationAnchorLink(), false);
		return;
	}

	// Get the target level path from the payload
	// Получаем путь целевого уровня из payload
	const FString TargetLevelPath = P->IsUseAnchor ? P->GetTargetLevelPackageName() : P->TargetLevelPath;
	bool IsUseTravel = P->IsUseTravel;

	if (IsUseTravel)
	{
		if (TargetLevelPath.IsEmpty())
		{
			OnTransitionCompleted.Broadcast(false, FLocationAnchorLink(), false);
			return;
		}

		const FGuid TargetAnchorID = P->GetTargetAnchorID();

		// Get the short name of the current level via GameplayStatics — the true parameter removes PIE prefixes
		// Получаем короткое имя текущего уровня через GameplayStatics — параметр true убирает PIE‑префиксы
		FString CurrentLevelShort = UGameplayStatics::GetCurrentLevelName(W, true);
		CurrentLevelShort = CurrentLevelShort.ToLower();

		// Function to normalize name from package → base filename
		// Функция нормализации имени из package → base filename
		auto NormalizeNameFromPackage = [](const FString& InPath) -> FString
			{
				if (InPath.IsEmpty()) return FString();

				FString PackagePath = InPath;
				if (PackagePath.Contains(TEXT(".")))
				{
					PackagePath = FPackageName::ObjectPathToPackageName(PackagePath);
				}

				const FString Base = FPaths::GetBaseFilename(PackagePath);

				FString Result = Base;
				int32 PIEPos = Result.Find(TEXT("UEDPIE_"), ESearchCase::IgnoreCase);
				if (PIEPos != INDEX_NONE)
				{
					int32 MPos = Result.Find(TEXT("_M_"), ESearchCase::IgnoreCase, ESearchDir::FromStart, PIEPos);
					if (MPos != INDEX_NONE && MPos + 3 < Result.Len())
						Result = Result.Mid(MPos + 3);
					else if (PIEPos + 6 < Result.Len())
						Result = Result.Mid(PIEPos + 6);
				}

				return Result.ToLower();
			};

		const FString NormTarget = NormalizeNameFromPackage(TargetLevelPath);

		// Compare short name of current level with normalized name of target level
		// Сравниваем короткое имя текущего уровня с нормализованным именем целевого уровня
		if (P->IsUseAnchor)
		{
			if (!CurrentLevelShort.IsEmpty() && NormTarget.IsEmpty() && CurrentLevelShort == NormTarget)
			{
				const bool bOk = TeleportToAnchor(TargetAnchorID);
				OnTransitionCompleted.Broadcast(bOk, TransitionPayloadCache->DestinationLink, false);
				return;
			}
			SetPendingAnchorID(TargetAnchorID);
		}

		APlayerController* PC = W->GetFirstPlayerController();
		if (PC && PC->GetClass()->ImplementsInterface(UPlayerController_I::StaticClass()))
		{
			IPlayerController_I::Execute_SetLoadScreen(PC, true);
			IPlayerController_I::Execute_StoreWeaponState(PC);
		}
	}
	FTimerDelegate TravelDelegate = FTimerDelegate::CreateLambda([TargetLevelPath, this]()
		{
			if (UWorld* LocalW = GetWorld())
				LocalW->SeamlessTravel(TargetLevelPath, true);
		});

	// Notify subscribers: we are about to leave the source floor
	// Уведомляем подписчиков: сейчас будет уход с исходного этажа
	// First notify via EventBus so that MissionSubsystem can request saving of mission snapshots.
	// Сначала оповестим через EventBus, чтобы MissionSubsystem мог запросить сохранение mission-snapshots.

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get() ? CachedEventBus.Get() : GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		// Reuse existing payload P (contains SourceFloor info)
		FOutcomeEventBase Ev;
		Ev.OutcomeType = EOutcomeType::Interior;
		Ev.OutcomeInterior = EOutcomeInterior::FloorLeaving; // notify listeners that we're leaving this floor / уведомить слушателей, что мы покидаем этаж
		Ev.Payload = P;
		EventBus->PublishOutcome(Ev);
	}
	// Then standard local broadcast / Затем стандартный локальный broadcast
	OnFloorExiting.Broadcast(P->SourceFloor);

	if (IsUseTravel)
	{
		UnsubscribeFromSpawnActor();
		W->GetTimerManager().SetTimer(TravelTimerHandle, TravelDelegate, 1.0f, false);
	}
}

// -----------------------------------------------------------------------------
// TeleportToAnchor
// Телепортация к якорю
// -----------------------------------------------------------------------------

bool UInteriorSubsystem::TeleportToAnchor(const FGuid& AnchorID)
{
	if (!AnchorID.IsValid()) return false;

	UWorld* W = GetWorld();
	if (!W) return false;

	ALocationAnchorActor* FoundAnchor = nullptr;
	for (TActorIterator<ALocationAnchorActor> It(W); It; ++It)
	{
		if (It->AnchorID == AnchorID)
		{
			FoundAnchor = *It;
			break;
		}
	}

	if (!FoundAnchor)
	{
		return false;
	}

	const FVector TargetLocation = FoundAnchor->GetActorLocation();
	const FRotator TargetRotation = FoundAnchor->GetActorRotation();

	APlayerController* PC = W->GetFirstPlayerController();
	if (!PC)
	{
		return false;
	}

	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->TeleportTo(TargetLocation, TargetRotation);
		PC->SetControlRotation(TargetRotation);
	}
	else
	{
		SetPendingSpawnTransform(TargetLocation, TargetRotation);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Pending Spawn Transform API
// API для отложенной трансформации спавна
// -----------------------------------------------------------------------------

void UInteriorSubsystem::SetPendingSpawnTransform(const FVector& Location, const FRotator& Rotation)
{
	bHasPendingSpawn = true;
	PendingSpawnLocation = Location;
	PendingSpawnRotation = Rotation;
}

void UInteriorSubsystem::GetPendingSpawnTransform(FVector& OutLocation, FRotator& OutRotation) const
{
	OutLocation = PendingSpawnLocation;
	OutRotation = PendingSpawnRotation;
}

void UInteriorSubsystem::ClearPendingSpawnTransform()
{
	bHasPendingSpawn = false;
	PendingSpawnLocation = FVector::ZeroVector;
	PendingSpawnRotation = FRotator::ZeroRotator;
}

bool UInteriorSubsystem::HasPendingSpawnTransform() const
{
	return bHasPendingSpawn;
}

// -----------------------------------------------------------------------------
// Pending Anchor ID API
// API для отложенного ID якоря
// -----------------------------------------------------------------------------

void UInteriorSubsystem::SetPendingAnchorID(const FGuid& AnchorID) { PendingAnchorID = AnchorID; }
FGuid UInteriorSubsystem::GetPendingAnchorID() const { return PendingAnchorID; }
void UInteriorSubsystem::ClearPendingAnchorID() { PendingAnchorID.Invalidate(); }
bool UInteriorSubsystem::HasPendingAnchorID() const { return PendingAnchorID.IsValid(); }

// -----------------------------------------------------------------------------
// Subscribe / Unsubscribe
// Подписка / отписка
// -----------------------------------------------------------------------------

void UInteriorSubsystem::SubscribeAll()
{
	UEventBusSubsystem* EventBus = CachedEventBus.Get();
	if (!EventBus) return;

	if (!SpawnRegisterHandle.IsValid())
	{
		SpawnConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		SpawnConditionAsset->OperatorType = EConditionOperator::Composite;
		SpawnConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		SpawnConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		SpawnConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractRegistered;
		SpawnConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		SpawnConditionAsset->CompileCondition();

		if (SpawnConditionAsset->GetCondition().IsValid())
		{
			SpawnRegisterHandle = EventBus->RegisterHandler(
				SpawnConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractRegistration));
		}
	}

	if (!DespawnRegisterHandle.IsValid())
	{
		DespawnConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		DespawnConditionAsset->OperatorType = EConditionOperator::Composite;
		DespawnConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		DespawnConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		DespawnConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractUnregistered;
		DespawnConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		DespawnConditionAsset->CompileCondition();

		if (DespawnConditionAsset->GetCondition().IsValid())
		{
			DespawnRegisterHandle = EventBus->RegisterHandler(
				DespawnConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractRegistration));
		}
	}

	SubscribePlacementRegistration();

	if (!InteractCommandHandle.IsValid())
	{
		InteractCommandConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		InteractCommandConditionAsset->OperatorType = EConditionOperator::Composite;
		InteractCommandConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		InteractCommandConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		InteractCommandConditionAsset->CompileCondition();

		if (InteractCommandConditionAsset->GetCondition().IsValid())
		{
			InteractCommandHandle = EventBus->RegisterHandler(
				InteractCommandConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractCommand));
		}
	}

	if (!SetEnabledHandle.IsValid())
	{
		SetEnabledConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		SetEnabledConditionAsset->OperatorType = EConditionOperator::Composite;
		SetEnabledConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		SetEnabledConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		SetEnabledConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractSetEnabled;
		SetEnabledConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		SetEnabledConditionAsset->CompileCondition();

		if (SetEnabledConditionAsset->GetCondition().IsValid())
		{
			SetEnabledHandle = EventBus->RegisterHandler(
				SetEnabledConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleSetEnabled));
		}
	}

	if (!SetRangeHandle.IsValid())
	{
		SetRangeConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		SetRangeConditionAsset->OperatorType = EConditionOperator::Composite;
		SetRangeConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		SetRangeConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		SetRangeConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractSetRange;
		SetRangeConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		SetRangeConditionAsset->CompileCondition();

		if (SetRangeConditionAsset->GetCondition().IsValid())
		{
			SetRangeHandle = EventBus->RegisterHandler(
				SetRangeConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleSetRange));
		}
	}

	if (!SetTooltipHandle.IsValid())
	{
		SetTooltipConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		SetTooltipConditionAsset->OperatorType = EConditionOperator::Composite;
		SetTooltipConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		SetTooltipConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		SetTooltipConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractSetTooltip;
		SetTooltipConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		SetTooltipConditionAsset->CompileCondition();

		if (SetTooltipConditionAsset->GetCondition().IsValid())
		{
			SetTooltipHandle = EventBus->RegisterHandler(
				SetTooltipConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleSetTooltip));
		}
	}

	if (!FloorStateSaveHandle.IsValid())
	{
		FloorStateSaveConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		FloorStateSaveConditionAsset->OperatorType = EConditionOperator::Composite;
		FloorStateSaveConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		FloorStateSaveConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		FloorStateSaveConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorStateSave;
		FloorStateSaveConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		FloorStateSaveConditionAsset->CompileCondition();

		if (FloorStateSaveConditionAsset->GetCondition().IsValid())
		{
			FloorStateSaveHandle = EventBus->RegisterHandler(
				FloorStateSaveConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleFloorStateSave));
		}
	}

	if (!FloorStateRestoreHandle.IsValid())
	{
		FloorStateRestoreConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		FloorStateRestoreConditionAsset->OperatorType = EConditionOperator::Composite;
		FloorStateRestoreConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		FloorStateRestoreConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		FloorStateRestoreConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorStateRestore;
		FloorStateRestoreConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		FloorStateRestoreConditionAsset->CompileCondition();

		if (FloorStateRestoreConditionAsset->GetCondition().IsValid())
		{
			FloorStateRestoreHandle = EventBus->RegisterHandler(
				FloorStateRestoreConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleFloorStateRestore));
		}
	}

	if (!FloorTransitionHandle.IsValid())
	{
		FloorTransitionConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		FloorTransitionConditionAsset->OperatorType = EConditionOperator::Composite;
		FloorTransitionConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		FloorTransitionConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		FloorTransitionConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorTransition;
		FloorTransitionConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		FloorTransitionConditionAsset->CompileCondition();

		if (FloorTransitionConditionAsset->GetCondition().IsValid())
		{
			FloorTransitionHandle = EventBus->RegisterHandler(
				FloorTransitionConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleFloorTransition));
		}
	}

	// Register handler for mission snapshot release commands (published by MissionSubsystem)
	// Регистрация обработчика для команд освобождения снимков миссий (публикуется MissionSubsystem)
	if (!MissionReleaseHandle.IsValid())
	{
		MissionReleaseConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		MissionReleaseConditionAsset->OperatorType = EConditionOperator::Composite;
		MissionReleaseConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		MissionReleaseConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		MissionReleaseConditionAsset->CompileCondition();

		if (MissionReleaseConditionAsset->GetCondition().IsValid())
		{
			MissionReleaseHandle = EventBus->RegisterHandler(
				MissionReleaseConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleReleaseMissionSnapshot));
		}
	}

	// Register handler for Update Mission List
	// Регистрация обработчика для обновления списка миссий
	if (!UpdateMissionListHandle.IsValid())
	{
		UpdateMissionListConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		UpdateMissionListConditionAsset->OperatorType = EConditionOperator::Composite;
		UpdateMissionListConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		UpdateMissionListConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		UpdateMissionListConditionAsset->CompileCondition();
		if (UpdateMissionListConditionAsset->GetCondition().IsValid())
		{
			UpdateMissionListHandle = EventBus->RegisterHandler(
				UpdateMissionListConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleUpdateMissionList));
		}
	}

	// Register handler for mission complete
	// Регистрация обработчика для завершения миссии
	if (!CompleteMissionHandle.IsValid())
	{
		CompleteMissionConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		CompleteMissionConditionAsset->OperatorType = EConditionOperator::Composite;
		CompleteMissionConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		CompleteMissionConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		CompleteMissionConditionAsset->FilterRow.MissionType = EOutcomeMission::MissionCompleted;
		CompleteMissionConditionAsset->FilterRow.MissionComparison = EConditionComparison::Equals;
		CompleteMissionConditionAsset->CompileCondition();
		if (CompleteMissionConditionAsset->GetCondition().IsValid())
		{
			CompleteMissionHandle = EventBus->RegisterHandler(
				CompleteMissionConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleCompleteMission));
		}
	}

}

void UInteriorSubsystem::SubscribePlacementRegistration()
{
	UEventBusSubsystem* EventBus = CachedEventBus.Get();
	if (!EventBus) return;

	if (!PlacementRegisterHandle.IsValid())
	{
		PlacementRegisterConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		PlacementRegisterConditionAsset->OperatorType = EConditionOperator::Composite;
		PlacementRegisterConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		PlacementRegisterConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		PlacementRegisterConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorPlacementRegistered;
		PlacementRegisterConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		PlacementRegisterConditionAsset->CompileCondition();

		if (PlacementRegisterConditionAsset->GetCondition().IsValid())
		{
			PlacementRegisterHandle = EventBus->RegisterHandler(
				PlacementRegisterConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandlePlacementRegistration));
		}
	}

	if (!PlacementUnregisterHandle.IsValid())
	{
		PlacementUnregisterConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		PlacementUnregisterConditionAsset->OperatorType = EConditionOperator::Composite;
		PlacementUnregisterConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		PlacementUnregisterConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		PlacementUnregisterConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorPlacementUnregistered;
		PlacementUnregisterConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		PlacementUnregisterConditionAsset->CompileCondition();

		if (PlacementUnregisterConditionAsset->GetCondition().IsValid())
		{
			PlacementUnregisterHandle = EventBus->RegisterHandler(
				PlacementUnregisterConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandlePlacementRegistration));
		}
	}
}

void UInteriorSubsystem::UnsubscribePlacementRegistration()
{
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		auto Unreg = [&](FOutcomeHandlerHandle& Handle, UOutcomeConditionAsset*& Asset)
			{
				if (Handle.IsValid()) { EventBus->UnregisterHandler(Handle); Handle.Invalidate(); }
				Asset = nullptr;
			};
		Unreg(PlacementRegisterHandle, PlacementRegisterConditionAsset);
		Unreg(PlacementUnregisterHandle, PlacementUnregisterConditionAsset);
	}
}
/*
void UInteriorSubsystem::ClearSpawnGroup(const FSpawnGroupId& GroupId, ESpawnGroupResolutionReason Reason)
{

}

void UInteriorSubsystem::ResetSpawnGroup(const FSpawnGroupId& GroupId)
{

}

FSpawnGroupState UInteriorSubsystem::GetSpawnGroupState(const FSpawnGroupId& GroupId) const
{

	return FSpawnGroupState();
}
*/
// -----------------------------------------------------------------------------
// Initialize / Deinitialize
// Инициализация / деинициализация
// -----------------------------------------------------------------------------

void UInteriorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Register with GameSaveSubsystem to participate in saving/loading
	// Регистрируемся в GameSaveSubsystem для участия в сохранении/загрузке
	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->RegisterSaveableSubsystem(this);
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		CachedEventBus = GI->GetSubsystem<UEventBusSubsystem>();
	}

	// Subscribe to map load (fires after SeamlessTravel)
	// Подписываемся на загрузку карты (срабатывает после SeamlessTravel)
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UInteriorSubsystem::OnPostLoadMap);

	SubscribeAll();
}

void UInteriorSubsystem::SubscribeToSpawnActor()
{
	if (UWorld* World = GetWorld())
	{
		ActorSpawnedHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UInteriorSubsystem::OnActorSpawned));
	}
}

void UInteriorSubsystem::UnsubscribeFromSpawnActor()
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}
}

void UInteriorSubsystem::Deinitialize()
{
	UnsubscribeAll();

	// Unsubscribe from map load
	// Отписываемся от загрузки карты
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	PostLoadMapHandle.Reset();

	RegisteredItems.Empty();
	RegistrationListeners.Empty();
	SpawnedActorsByInteriorFloor.Empty();
	MissionFloorSnapshots.Empty();
	CachedEventBus = nullptr;

	UnsubscribeFromSpawnActor();

	Super::Deinitialize();
}

// New function — map load completion handler
// Новая функция — обработчик завершения загрузки карты
void UInteriorSubsystem::OnPostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld) return;

	// Check that this is a game world, not PIE preview or editor
	// Проверяем что это игровой мир, а не PIE preview или editor
	if (LoadedWorld->WorldType != EWorldType::Game &&
		LoadedWorld->WorldType != EWorldType::PIE)
	{
		return;
	}

	//SubscribeToSpawnActor();

	bool bNeedTeleportToAnchor = true;
	UGameInstance* GI = UGameplayStatics::GetGameInstance(LoadedWorld);
	if (GI && GI->GetClass()->ImplementsInterface(USceneDataProvider::StaticClass()))
	{
		bNeedTeleportToAnchor = ISceneDataProvider::Execute_GetNeedTeleportToAnchor(GI);
	}


	if (bNeedTeleportToAnchor && !HasPendingAnchorID())
	{
		return;
	}

	const FGuid AnchorID = GetPendingAnchorID();

	// Small delay — wait for actors to be created on the level
	// Небольшая задержка — дождаться создания акторов на уровне
	FTimerHandle TimerHandle;
	FTimerDelegate Delegate = FTimerDelegate::CreateLambda([this, AnchorID, LoadedWorld, bNeedTeleportToAnchor]()
		{
			// Try to teleport — if false, still continue checking (anchor may be missing, but restore still not performed).
			// Попытка телепорта — если false, всё равно продолжим проверку (якорь мог отсутствовать, но restore всё равно не делать).
			bool bOk = false;

			if (bNeedTeleportToAnchor)
			{
				bOk = TeleportToAnchor(AnchorID);
			}

			UGameInstance* GI = UGameplayStatics::GetGameInstance(LoadedWorld);
			if (GI && GI->GetClass()->ImplementsInterface(USceneDataProvider::StaticClass()))
			{
				ISceneDataProvider::Execute_SetNeedTeleportToAnchor(GI, true);
			}

			// Try to find the anchor actor in the current world
			// Пытаемся найти актор-якорь в текущем мире
			UWorld* World = GetWorld();
			ALocationAnchorActor* FoundAnchor = nullptr;
			if (World)
			{
				for (TActorIterator<ALocationAnchorActor> It(World); It; ++It)
				{
					if (bNeedTeleportToAnchor)
					{
						if (It->AnchorID == AnchorID)
						{
							FoundAnchor = *It;
							break;
						}
					}
					else
					{
						FoundAnchor = *It;
						break;
					}
				}
			}

			// If anchor found — extract the associated FloorAsset (if any) and form CurrentKey
			// Если нашли якорь — извлекаем связанный FloorAsset (если есть) и формируем CurrentKey

			bool bHaveKey = false;
			if (FoundAnchor)
			{
				UFloorAsset* FloorAsset = FoundAnchor->OwnerFloor.LoadSynchronous();
				UInteriorSetAsset* InteriorAsset = FoundAnchor->OwnerInteriorSet.LoadSynchronous();
				if (InteriorAsset && InteriorAsset->InteriorSetID.IsValid())
				{
					FGuid InteriorSetId = InteriorAsset->InteriorSetID;

					if (FloorAsset && FloorAsset->FloorID.IsValid())
					{
						FGuid FloorId = FloorAsset->FloorID;
						CurrentKey = FInteriorFloorKey(InteriorSetId, FloorId);
						bHaveKey = true;
					}
				}
			}
			// Inside the lambda, after determining bHaveKey and CurrentKey
			// Внутри лямбды, после того как определили bHaveKey и CurrentKey
			if (bHaveKey)
			{
				//SubscribePlacementRegistration();
				SubscribeToSpawnActor();
				// 2. Restore base snapshots (FloorStateSnapshots)
				// 2. Восстанавливаем базовые снапшоты состояния (FloorStateSnapshots)
				FMissionEnvelope FindedEnvelope;
				if (const TArray<FFloorSavedActorState>* BaseSnap = FloorStateSnapshots.Find(CurrentKey))
				{
					if (BaseSnap && !BaseSnap->IsEmpty())
					{

						if (!World) return;

						bool IsMissionWorld = false;

						FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
						FString MissionName;
						FName FindedMissionId;

						for (const auto& Pair : ActiveMissions)
						{
							const UMissionController* Ctrl = Pair.Value.Controller;
							if (!Ctrl) continue;

							const UMissionAsset* Asset = Ctrl->GetMissionAsset();
							if (!Asset) continue;

							MissionName = Asset->Description.ToString();
							const EMissionStatus Status = Ctrl->GetStatus();
							const EMissionEndReason EndReason = Ctrl->GetEndReason();
							const FMissionEnvelope Envelope = Asset->Envelopes[Pair.Value.MissionStep];
							const EMissionResumeMode Resume = Envelope.ResumeMode;
							const int32 MissionStep = Pair.Value.MissionStep;

							FMissionEnvelopeScope Scope = Envelope.Scope;
							for (auto Sc : Scope.InteriorScopes)
							{
								FString ScopeLevelName = Sc->FloorLevel.ToSoftObjectPath().GetLongPackageName();

								FString NormTarget = NormalizeLevelName(ScopeLevelName);
								FString NormCurrent = NormalizeLevelName(CurrentLevelName);

								if (NormTarget == NormCurrent)
								{
									IsMissionWorld = true;
									FindedMissionId = Pair.Key;
									FindedEnvelope = Envelope;
									break;
								}
							}

							if (IsMissionWorld)
							{
								break;
							}
						}

						RestoreSpawnedActorsForCurrentFloor(FMissionEnvelope());

						RestoreFromSnapshotArray(World, *BaseSnap, CurrentKey.InteriorSetId, CurrentKey.FloorId, FMissionEnvelope());

					}
				}

				// 3. Restore mission snapshots (if a mission is active)
				// 3. Восстанавливаем миссионные снапшоты (если активна миссия)
				if (const TMap<FName, TArray<FFloorSavedActorState>>* PerFloor = MissionFloorSnapshots.Find(CurrentKey))
				{
					if (PerFloor->Num() > 0)
					{

						bool IsMissionWorld = false;

						FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
						FString MissionName;
						FName FindedMissionId;

						for (const auto& Pair : ActiveMissions)
						{
							const UMissionController* Ctrl = Pair.Value.Controller;
							if (!Ctrl) continue;

							const UMissionAsset* Asset = Ctrl->GetMissionAsset();
							if (!Asset) continue;

							MissionName = Asset->Description.ToString();
							const EMissionStatus Status = Ctrl->GetStatus();
							const EMissionEndReason EndReason = Ctrl->GetEndReason();
							const FMissionEnvelope Envelope = Asset->Envelopes[Pair.Value.MissionStep];
							const EMissionResumeMode Resume = Envelope.ResumeMode;
							const int32 MissionStep = Pair.Value.MissionStep;

							FMissionEnvelopeScope Scope = Envelope.Scope;
							for (auto Sc : Scope.InteriorScopes)
							{
								FString ScopeLevelName = Sc->FloorLevel.ToSoftObjectPath().GetLongPackageName();

								FString NormTarget = NormalizeLevelName(ScopeLevelName);
								FString NormCurrent = NormalizeLevelName(CurrentLevelName);

								if (NormTarget == NormCurrent)
								{
									IsMissionWorld = true;
									FindedMissionId = Pair.Key;
									FindedEnvelope = Envelope;
									break;
								}
							}

							if (IsMissionWorld)
							{
								break;
							}
						}


						for (const auto& MissionPair : *PerFloor)
						{

							const FName& MissionId = MissionPair.Key;
							SpawnMissionActorsFromCurrentFloor(MissionId);

							if (ActiveMissions.Contains(MissionId))
							{
								int32 CurrentMissionStep = ActiveMissions[MissionId].MissionStep;
								RestoreMissionFloorState(MissionId, CurrentMissionStep, CurrentKey);
							}
						}
					}
				}
			}

			bool IsMissionWorld = false;

			FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
			FString MissionName;
			FName FindedMissionId;

			for (const auto& Pair : ActiveMissions)
			{
				const UMissionController* Ctrl = Pair.Value.Controller;
				if (!Ctrl) continue;

				const UMissionAsset* Asset = Ctrl->GetMissionAsset();
				if (!Asset) continue;

				MissionName = Asset->Description.ToString();
				const EMissionStatus Status = Ctrl->GetStatus();
				const EMissionEndReason EndReason = Ctrl->GetEndReason();
				const FMissionEnvelope Envelope = Asset->Envelopes[Pair.Value.MissionStep];
				const EMissionResumeMode Resume = Envelope.ResumeMode;
				const int32 MissionStep = Pair.Value.MissionStep;

				FMissionEnvelopeScope Scope = Envelope.Scope;
				for (auto Sc : Scope.InteriorScopes)
				{
					FString ScopeLevelName = Sc->FloorLevel.ToSoftObjectPath().GetLongPackageName();

					FString NormTarget = NormalizeLevelName(ScopeLevelName);
					FString NormCurrent = NormalizeLevelName(CurrentLevelName);

					if (NormTarget == NormCurrent)
					{
						IsMissionWorld = true;
						FindedMissionId = Pair.Key;
						break;
					}
				}

				if (IsMissionWorld)
				{
					break;
				}
			}

			if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
			{
				UUpdateActiveMissionId* Payload = EventBus->CreatePayload<UUpdateActiveMissionId>();
				if (Payload)
				{
					Payload->ActiveMissionId = FindedMissionId;
					FOutcomeEventBase Ev;
					Ev.OutcomeType = EOutcomeType::Mission;
					Ev.OutcomeMission = EOutcomeMission::MissionUpdate;
					Ev.Payload = Payload;
					EventBus->PublishOutcome(Ev);
				}
			}

			// Notification about the end of level loading 
			// Оповещение о конце загрузки уровня
			if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
			{
				ULevelLoadedPayload* Payload = EventBus->CreatePayload<ULevelLoadedPayload>();
				if (Payload)
				{
					FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
					FString PackageName = LoadedWorld->GetOutermost()->GetName();
					Payload->Setup(LevelName, PackageName);

					FOutcomeEventBase Ev;
					Ev.OutcomeType = EOutcomeType::Interior;
					Ev.OutcomeInterior = EOutcomeInterior::LevelLoaded;
					Ev.Payload = Payload;
					EventBus->PublishOutcome(Ev);
					UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Published LevelLoaded event for level %s"), *LevelName);
				}
			}

			// Final notification about completion of loading/transition
			// Финальная нотификация о завершении загрузки/перехода
			OnTransitionCompleted.Broadcast(bOk, TransitionPayloadCache ? TransitionPayloadCache->DestinationLink : FLocationAnchorLink(), true);
			ClearPendingAnchorID();
		});

	LoadedWorld->GetTimerManager().SetTimer(TimerHandle, Delegate, 1.0f, false);
}

void UInteriorSubsystem::SubscribeInteractionRegistration() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeInteractionRegistration() { UnsubscribeAll(); }

void UInteriorSubsystem::SubscribeInteractCommand() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeInteractCommand() { UnsubscribeAll(); }

void UInteriorSubsystem::SubscribeSetEnabled() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeSetEnabled() { UnsubscribeAll(); }

void UInteriorSubsystem::SubscribeSetRange() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeSetRange() { UnsubscribeAll(); }

void UInteriorSubsystem::SubscribeSetTooltip() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeSetTooltip() { UnsubscribeAll(); }

// Returns the list of placed actors for InteriorSetId/FloorId (const)
// Возвращает список размещённых акторов для InteriorSetId/FloorId (const)
TArray<FFloorPopulationRecord> UInteriorSubsystem::GetPlacedActorsForInteriorFloor(const FGuid& InteriorSetId, const FGuid& FloorId) const
{
	TArray<FFloorPopulationRecord> Result;
	const FInteriorFloorKey Key(InteriorSetId, FloorId);
	if (const FFloorPopulationBuckets* Buckets = SpawnedActorsByInteriorFloor.Find(Key))
	{
		Result.Append(Buckets->HeavyFurniture);
		Result.Append(Buckets->LightItems);
		Result.Append(Buckets->Terminals);
		Result.Append(Buckets->NPCSpawners);
		Result.Append(Buckets->Debris);
	}
	return Result;
}

void UInteriorSubsystem::AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	FInteractiveSubsystemMethods::AddRegistrationListener(ItemId, Listener);
}

void UInteriorSubsystem::RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	FInteractiveSubsystemMethods::RemoveRegistrationListener(ItemId, Listener);
}

// Implementation of the protected method required by FInteractiveSubsystemMethods
// Реализация защищённого метода, требуемого FInteractiveSubsystemMethods
TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& UInteriorSubsystem::GetRegistrationListeners()
{
	return RegistrationListeners;
}

void UInteriorSubsystem::UnsubscribeAll()
{
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		auto UnregisterHandle = [&](FOutcomeHandlerHandle& Handle, UOutcomeConditionAsset*& Asset)
			{
				if (Handle.IsValid())
				{
					EventBus->UnregisterHandler(Handle);
					Handle.Invalidate();
				}
				Asset = nullptr;
			};

		UnregisterHandle(SpawnRegisterHandle, SpawnConditionAsset);
		UnregisterHandle(DespawnRegisterHandle, DespawnConditionAsset);

		UnregisterHandle(PlacementRegisterHandle, PlacementRegisterConditionAsset);
		UnregisterHandle(PlacementUnregisterHandle, PlacementUnregisterConditionAsset);

		UnregisterHandle(InteractCommandHandle, InteractCommandConditionAsset);
		UnregisterHandle(SetEnabledHandle, SetEnabledConditionAsset);
		UnregisterHandle(SetRangeHandle, SetRangeConditionAsset);
		UnregisterHandle(SetTooltipHandle, SetTooltipConditionAsset);

		UnregisterHandle(FloorStateSaveHandle, FloorStateSaveConditionAsset);
		UnregisterHandle(FloorStateRestoreHandle, FloorStateRestoreConditionAsset);
		UnregisterHandle(FloorTransitionHandle, FloorTransitionConditionAsset);
	}

	RegisteredItems.Empty();
	RegistrationListeners.Empty();
}

// ─────────────────────────────────────────────────────────────────────────────
// Mission Snapshot API
// API для снимков миссий
// ─────────────────────────────────────────────────────────────────────────────
void UInteriorSubsystem::RestoreMissionFloorState(FName MissionId, int32 CurrentMissionStep, const FInteriorFloorKey& FloorKey)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (MissionId.IsNone() || !FloorKey.FloorId.IsValid()) return;

	// Find per-floor map, then entry by MissionId
	// Найти per-floor map, затем entry по MissionId
	if (const TMap<FName, TArray<FFloorSavedActorState>>* PerFloor = MissionFloorSnapshots.Find(FloorKey))
	{
		if (const TArray<FFloorSavedActorState>* Snapshot = PerFloor->Find(MissionId))
		{
			if (!Snapshot || Snapshot->IsEmpty()) return;

			auto MissionInterior = ActiveMissions.Find(MissionId);
			auto& Controller = MissionInterior->Controller;
			auto MissionAsset = Controller->GetMissionAsset();
			auto& Envelope = MissionAsset->Envelopes[CurrentMissionStep];

			RestoreFromSnapshotArray(World, *Snapshot, FloorKey.InteriorSetId, FloorKey.FloorId, Envelope);
		}
	}
}

void UInteriorSubsystem::FloorSpawnActorsFromType(TArray<FFloorPopulationRecord>& Array, const FMissionEnvelope& Envelope, FInteriorFloorKey Key)
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (const FFloorPopulationRecord& Record : Array)
	{
		if (!Record.SourceClass) continue;
		EEnvelopeChannel Channel = FloorActorTypeToEnvelopeChannel(Record.ActorType);

		TArray<FFloorSavedActorState>* FloorArray = FloorStateSnapshots.Find(Key);
		if (FloorArray)
		{
			bool bExists = FloorArray->ContainsByPredicate([&](const FFloorSavedActorState& State)
				{
					return State.ItemId == Record.ActorId;
				});

			if (!bExists)
			{
				if (FindActorByItemId(Record.ActorId)) continue;
				FActorSpawnParameters Params;
				AActor* SpawnedActor = World->SpawnActor<AActor>(Record.SourceClass, Record.WorldTransform, Params);
				if (!SpawnedActor) continue;
				UFloorAssignmentComponent* Comp = SpawnedActor->FindComponentByClass<UFloorAssignmentComponent>();
				if (Comp)
				{
					Comp->ItemId = Record.ActorId;
					Comp->SnapshotChannel = ESnapshotChannel::Snapshot;
					Comp->ActorType = Record.ActorType;
				}
			}
		}

		bool IsAlreadySpawned = false;
		TArray<AActor*> AllActors;
		UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
		for (AActor* Actor : AllActors)
		{
			if (!IsValid(Actor)) continue;
			if (UFloorAssignmentComponent* C = Actor->FindComponentByClass<UFloorAssignmentComponent>())
			{
				if (Record.ActorId == C->ItemId)
				{
					IsAlreadySpawned = true;
					break;
				}
			}
		}

		if (IsAlreadySpawned) continue;

		FActorSpawnParameters Params;
		AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(Record.SourceClass, Record.WorldTransform, Params);
		if (!SpawnedActor) continue;

		// Find the FloorAssignment component and set its properties (required!)
		// Найти компонент FloorAssignment и установить его свойства (обязательно!)
		UFloorAssignmentComponent* Comp = SpawnedActor->FindComponentByClass<UFloorAssignmentComponent>();
		if (Comp)
		{
			Comp->ItemId = Record.ActorId;
			Comp->SnapshotChannel = ESnapshotChannel::Snapshot;
			Comp->ActorType = Record.ActorType;
		}
	}
}

void UInteriorSubsystem::SpawnMissionActorsFromCurrentFloor(FName MissionId)
{
	UWorld* World = GetWorld();
	if (!World) return;

	bool IsMissionWorld = false;

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	FString MissionName;
	FName FindedMissionId;
	FMissionEnvelope FindedEnvelope;

	for (const auto& Pair : ActiveMissions)
	{
		const UMissionController* Ctrl = Pair.Value.Controller;
		if (!Ctrl) continue;

		const UMissionAsset* Asset = Ctrl->GetMissionAsset();
		if (!Asset) continue;

		MissionName = Asset->Description.ToString();
		const EMissionStatus Status = Ctrl->GetStatus();
		const EMissionEndReason EndReason = Ctrl->GetEndReason();
		const FMissionEnvelope Envelope = Asset->Envelopes[Pair.Value.MissionStep];
		const EMissionResumeMode Resume = Envelope.ResumeMode;
		const int32 MissionStep = Pair.Value.MissionStep;

		FMissionEnvelopeScope Scope = Envelope.Scope;
		for (auto Sc : Scope.InteriorScopes)
		{
			FString ScopeLevelName = Sc->FloorLevel.ToSoftObjectPath().GetLongPackageName();

			FString NormTarget = NormalizeLevelName(ScopeLevelName);
			FString NormCurrent = NormalizeLevelName(CurrentLevelName);

			if (NormTarget == NormCurrent)
			{
				IsMissionWorld = true;
				FindedMissionId = Pair.Key;
				FindedEnvelope = Envelope;
				break;
			}
		}

		if (IsMissionWorld)
		{
			break;
		}
	}

	if (IsMissionWorld)
	{
		// Apply spawns that occurred during the mission
		// Применяем спавны, произошедшие во время миссии
		if (auto* SpawnedMapPerFloor = MissionSpawnedActorsByInteriorFloor.Find(CurrentKey))
		{
			if (auto* BucketsForMission = SpawnedMapPerFloor->Find(MissionId))
			{
				auto RemoveRecordByActorId = [&](TArray<FFloorPopulationRecord>& Arr, const FGuid& ItemId)
					{
						Arr.RemoveAll([&ItemId](const FFloorPopulationRecord& Rec) { return Rec.ActorId == ItemId; });
					};

				auto ProcessCategory = [&](TArray<FFloorPopulationRecord>& Records, EFloorActorType ActorType)
					{
						if (ShouldUseActor(FindedEnvelope, FloorActorTypeToEnvelopeChannel(ActorType), EMissionEndReason::None, true))
						{
							SpawnMissionRecords(Records);
						}
						else
						{
							Records.Empty();
						}
					};

				ProcessCategory(BucketsForMission->HeavyFurniture, EFloorActorType::HeavyFurniture);
				ProcessCategory(BucketsForMission->LightItems, EFloorActorType::LightItem);
				ProcessCategory(BucketsForMission->Terminals, EFloorActorType::Terminal);
				ProcessCategory(BucketsForMission->NPCSpawners, EFloorActorType::SpawnGroupSpawner);
				ProcessCategory(BucketsForMission->Debris, EFloorActorType::Debris);
			}
		}
		if (auto* DestroyedMapPerFloor = MissionDestroyedActorsByInteriorFloor.Find(CurrentKey))
		{
			if (auto* BucketsForMission = DestroyedMapPerFloor->Find(MissionId))
			{
				auto ProcessCategory = [&](TArray<FFloorPopulationRecord>& Records, EFloorActorType ActorType)
					{
						if (ShouldUseActor(FindedEnvelope, FloorActorTypeToEnvelopeChannel(ActorType), EMissionEndReason::None, true))
							DestroyMissionRecords(Records);
						else
							Records.Empty();
					};

				ProcessCategory(BucketsForMission->HeavyFurniture, EFloorActorType::HeavyFurniture);
				ProcessCategory(BucketsForMission->LightItems, EFloorActorType::LightItem);
				ProcessCategory(BucketsForMission->Terminals, EFloorActorType::Terminal);
				ProcessCategory(BucketsForMission->NPCSpawners, EFloorActorType::SpawnGroupSpawner);
				ProcessCategory(BucketsForMission->Debris, EFloorActorType::Debris);
			}
		}
	}
}

void UInteriorSubsystem::SpawnMissionRecords(const TArray<FFloorPopulationRecord>& Records)
{
	UWorld* World = GetWorld();
	if (!World) return;
	for (const FFloorPopulationRecord& Record : Records)
	{
		if (!Record.SourceClass) continue;
		// Check if the object already exists (via FindActorByItemId)
		// Проверка, что объект ещё не существует (можно через FindActorByItemId)
		if (FindActorByItemId(Record.ActorId)) continue;
		FActorSpawnParameters Params;
		AActor* SpawnedActor = World->SpawnActor<AActor>(Record.SourceClass, Record.WorldTransform, Params);
		if (!SpawnedActor) continue;
		UFloorAssignmentComponent* Comp = SpawnedActor->FindComponentByClass<UFloorAssignmentComponent>();
		if (Comp)
		{
			Comp->ItemId = Record.ActorId;
			Comp->SnapshotChannel = ESnapshotChannel::Snapshot;
			Comp->ActorType = Record.ActorType;
		}
	}
}

void UInteriorSubsystem::DestroyMissionRecords(const TArray<FFloorPopulationRecord>& Records)
{
	for (const FFloorPopulationRecord& Record : Records)
	{
		AActor* Actor = FindActorByItemId(Record.ActorId);
		if (Actor && IsValid(Actor))
			Actor->Destroy();
	}
}

void UInteriorSubsystem::RestoreSpawnedActorsForCurrentFloor(const FMissionEnvelope& Envelope)
{
	TMap<FInteriorFloorKey, FFloorPopulationBuckets> Temp = SpawnedActorsByInteriorFloor;
	FFloorPopulationBuckets* Buckets = Temp.Find(CurrentKey);
	if (!Buckets) return;

	UWorld* World = GetWorld();

	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), TestAllActors);

	// Simply spawn actors without restoring state (state will be restored later from MissionFloorSnapshots)
	// Просто спавним акторы без восстановления состояния (состояние будет восстановлено позже из MissionFloorSnapshots)
	FloorSpawnActorsFromType(Buckets->HeavyFurniture, Envelope, CurrentKey);
	FloorSpawnActorsFromType(Buckets->LightItems, Envelope, CurrentKey);
	FloorSpawnActorsFromType(Buckets->Terminals, Envelope, CurrentKey);
	FloorSpawnActorsFromType(Buckets->NPCSpawners, Envelope, CurrentKey);
	FloorSpawnActorsFromType(Buckets->Debris, Envelope, CurrentKey);

	TestAllActors.Empty();
}

void UInteriorSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
	UWorld* World = GetWorld();
	if (!World) return;

	bool IsMissionWorld = false;

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	FString MissionName;
	FName FindedMissionId;
	FMissionEnvelope FindedEnvelope;
	for (const auto& Pair : ActiveMissions)
	{
		const UMissionController* Ctrl = Pair.Value.Controller;
		if (!Ctrl) continue;

		const UMissionAsset* Asset = Ctrl->GetMissionAsset();
		if (!Asset) continue;

		MissionName = Asset->Description.ToString();
		const EMissionStatus Status = Ctrl->GetStatus();
		const EMissionEndReason EndReason = Ctrl->GetEndReason();
		const FMissionEnvelope Envelope = Asset->Envelopes[Pair.Value.MissionStep];
		const EMissionResumeMode Resume = Envelope.ResumeMode;
		const int32 MissionStep = Pair.Value.MissionStep;

		FMissionEnvelopeScope Scope = Envelope.Scope;
		for (auto Sc : Scope.InteriorScopes)
		{
			FString ScopeLevelName = Sc->FloorLevel.ToSoftObjectPath().GetLongPackageName();

			FString NormTarget = NormalizeLevelName(ScopeLevelName);
			FString NormCurrent = NormalizeLevelName(CurrentLevelName);

			if (NormTarget == NormCurrent)
			{
				IsMissionWorld = true;
				FindedMissionId = Pair.Key;
				FindedEnvelope = Envelope;
				break;
			}
		}

		if (IsMissionWorld)
		{
			break;
		}
	}

	StoreCurrentLevel(FindedEnvelope, FindedMissionId, EMissionEndReason::None);
	//StoreSnapshot(FindedMissionId, FindedEnvelope, EMissionEndReason::None);


	OutData.SubsystemName = GetSaveSubsystemName();

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	// --- Serialize ActiveMissions (as before, with minor fixes)
	// --- Сериализация ActiveMissions (как было ранее, но с небольшими правками)
	TArray<TSharedPtr<FJsonValue>> MissionArray;
	for (const auto& Pair : ActiveMissions)
	{
		const UMissionController* Ctrl = Pair.Value.Controller;
		if (!Ctrl) continue;

		const UMissionAsset* Asset = Ctrl->GetMissionAsset();
		if (!Asset) continue;

		const EMissionStatus Status = Ctrl->GetStatus();
		const EMissionEndReason EndReason = Ctrl->GetEndReason();
		const FMissionEnvelope& Envelope = Asset->Envelopes[Pair.Value.MissionStep];
		const EMissionResumeMode Resume = Envelope.ResumeMode;
		const int32 MissionStep = Pair.Value.MissionStep;

		uint8 SavedStatus = static_cast<uint8>(Status);
		uint8 SavedEndReason = static_cast<uint8>(EndReason);

		if (Resume == EMissionResumeMode::FailOnLoad && Status == EMissionStatus::Active)
		{
			SavedStatus = static_cast<uint8>(EMissionStatus::Resolved);
			SavedEndReason = static_cast<uint8>(EMissionEndReason::Failed);
		}

		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("MissionId"), Pair.Key.ToString());
		Obj->SetStringField(TEXT("AssetPath"), Asset->GetPathName());
		Obj->SetNumberField(TEXT("Status"), SavedStatus);
		Obj->SetNumberField(TEXT("EndReason"), SavedEndReason);
		Obj->SetNumberField(TEXT("ResumeMode"), static_cast<uint8>(Resume));
		Obj->SetNumberField(TEXT("MissionStep"), static_cast<uint8>(MissionStep));

		MissionArray.Add(MakeShared<FJsonValueObject>(Obj));
	}
	Root->SetArrayField(TEXT("Missions"), MissionArray);

	// --- Serialize FloorStateSnapshots ---
	// --- Сериализация FloorStateSnapshots ---
	Root->SetField(TEXT("FloorStateSnapshots"), SerializeFloorStateSnapshots(FloorStateSnapshots));

	// --- Serialize MissionFloorSnapshots ---
	// --- Сериализация MissionFloorSnapshots ---
	Root->SetField(TEXT("MissionFloorSnapshots"), SerializeMissionFloorSnapshots(MissionFloorSnapshots));

	// --- Serialize SpawnedActorsByInteriorFloor and DestroyedActorsByInteriorFloor ---
	// --- Сериализация SpawnedActorsByInteriorFloor и DestroyedActorsByInteriorFloor ---
	Root->SetField(TEXT("SpawnedActors"), SerializePopulationMap(SpawnedActorsByInteriorFloor));
	Root->SetField(TEXT("DestroyedActors"), SerializePopulationMap(DestroyedActorsByInteriorFloor));

	Root->SetField(TEXT("MissionSpawnedActors"), SerializeMissionPopulationMap(MissionSpawnedActorsByInteriorFloor));
	Root->SetField(TEXT("MissionDestroyedActors"), SerializeMissionPopulationMap(MissionDestroyedActorsByInteriorFloor));

	// Write to string
	// Запись в строку
	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	OutData.SerializedData = Output;
}

void UInteriorSubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
	IsLoadComplete = false;

	if (InData.SerializedData.IsEmpty())
	{
		IsLoadComplete = true;
		return;
	}

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		IsLoadComplete = true;
		return;
	}

	// --- Restore FloorStateSnapshots ---
	// --- Восстановление FloorStateSnapshots ---
	TSharedPtr<FJsonValue> FloorStateValue = Root->TryGetField(TEXT("FloorStateSnapshots"));
	if (FloorStateValue.IsValid())
	{
		DeserializeFloorStateSnapshots(FloorStateValue, FloorStateSnapshots);
	}

	// --- Restore MissionFloorSnapshots ---
	// --- Восстановление MissionFloorSnapshots ---
	TSharedPtr<FJsonValue> MissionFloorValue = Root->TryGetField(TEXT("MissionFloorSnapshots"));
	if (MissionFloorValue.IsValid())
	{
		MissionFloorSnapshots.Empty();
		DeserializeMissionFloorSnapshots(MissionFloorValue, MissionFloorSnapshots);
	}

	// --- Deserialize SpawnedActorsByInteriorFloor and DestroyedActorsByInteriorFloor ---
	// --- Десериализация SpawnedActorsByInteriorFloor и DestroyedActorsByInteriorFloor ---
	TSharedPtr<FJsonValue> SpawnedValue = Root->TryGetField(TEXT("SpawnedActors"));
	if (SpawnedValue.IsValid())
	{
		DeserializePopulationMap(SpawnedValue, SpawnedActorsByInteriorFloor);
	}
	TSharedPtr<FJsonValue> DestroyedValue = Root->TryGetField(TEXT("DestroyedActors"));
	if (DestroyedValue.IsValid())
	{
		DeserializePopulationMap(DestroyedValue, DestroyedActorsByInteriorFloor);
	}

	TSharedPtr<FJsonValue> MissionSpawnedValue = Root->TryGetField(TEXT("MissionSpawnedActors"));
	if (MissionSpawnedValue.IsValid())
	{
		DeserializeMissionPopulationMap(MissionSpawnedValue, MissionSpawnedActorsByInteriorFloor);
	}
	TSharedPtr<FJsonValue> MissionDestroyedValue = Root->TryGetField(TEXT("MissionDestroyedActors"));
	if (MissionDestroyedValue.IsValid())
	{
		DeserializeMissionPopulationMap(MissionDestroyedValue, MissionDestroyedActorsByInteriorFloor);
	}

	// --- Restore list of active missions ---
	// --- Восстановление списка активных миссий ---
	const TArray<TSharedPtr<FJsonValue>>* MissionArray = nullptr;
	ActiveMissions.Empty();

	if (Root->TryGetArrayField(TEXT("Missions"), MissionArray))
	{
		for (const TSharedPtr<FJsonValue>& Val : *MissionArray)
		{
			const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
			if (!Val->TryGetObject(ObjPtr)) continue;
			const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

			FString MissionIdStr, AssetPath;
			int32 StatusInt = 0, EndReasonInt = 0, ResumeModeInt = 0, MissionStep = -1;

			Obj->TryGetStringField(TEXT("MissionId"), MissionIdStr);
			Obj->TryGetStringField(TEXT("AssetPath"), AssetPath);
			Obj->TryGetNumberField(TEXT("Status"), StatusInt);
			Obj->TryGetNumberField(TEXT("EndReason"), EndReasonInt);
			Obj->TryGetNumberField(TEXT("ResumeMode"), ResumeModeInt);
			Obj->TryGetNumberField(TEXT("MissionStep"), MissionStep);

			const FName MissionId = FName(*MissionIdStr);
			const EMissionEndReason EndReason = static_cast<EMissionEndReason>(EndReasonInt);
			const EMissionResumeMode Resume = static_cast<EMissionResumeMode>(ResumeModeInt);

			UMissionAsset* Asset = Cast<UMissionAsset>(
				StaticLoadObject(UMissionAsset::StaticClass(), nullptr, *AssetPath));
			if (!Asset) continue;

			switch (Resume)
			{
			case EMissionResumeMode::RestartOnLoad:
			{
				UMissionController* Ctrl = CreateMission(Asset);
				if (Ctrl)
				{
					Ctrl->MissionStep = 0;
					FActiveMissionInterior* Entry = ActiveMissions.Find(MissionId);
					if (Entry) Entry->MissionStep = Ctrl->MissionStep;
					// Remove snapshots of the restarted mission
					// Удаляем снепшоты перезапущенной миссии
					RemoveMissionSnapshots(MissionFloorSnapshots, MissionId);
					RemoveMissionFromPopulationMap(MissionSpawnedActorsByInteriorFloor, MissionId);
					RemoveMissionFromPopulationMap(MissionDestroyedActorsByInteriorFloor, MissionId);
				}
				Ctrl->OnMissionStepProgress(0);
				break;
			}
			case EMissionResumeMode::Resumable:
			{
				UMissionController* Ctrl = CreateMission(Asset);
				if (Ctrl)
				{
					Ctrl->MissionStep = MissionStep;
					FActiveMissionInterior* Entry = ActiveMissions.Find(MissionId);
					if (Entry) Entry->MissionStep = Ctrl->MissionStep;
				}
				break;
			}
			case EMissionResumeMode::FailOnLoad:
			{
				UMissionController* Ctrl = CreateMission(Asset);
				if (!Ctrl) continue;
				Ctrl->MissionStep = MissionStep;
				FActiveMissionInterior* Entry = ActiveMissions.Find(MissionId);
				if (Entry) Entry->MissionStep = Ctrl->MissionStep;
				// Defer processing (snapshot transfer) until the end
				// Отложим обработку (перенос снепшотов) до конца

				Ctrl->OnMissionCompleted(EMissionEndReason::Failed);
				break;
			}
			}
		}

		// --- Process missions with ResumeMode == FailOnLoad ---
		// --- Обработка миссий с ResumeMode == FailOnLoad ---
		for (auto& Pair : ActiveMissions)
		{
			const FName MissionId = Pair.Key;
			UMissionController* Controller = Pair.Value.Controller;
			if (!Controller) continue;

			UMissionAsset* MissionAsset = Controller->GetMissionAsset();
			if (!MissionAsset) continue;

			FMissionEnvelope Envelope = MissionAsset->Envelopes[Pair.Value.MissionStep];
			if (Envelope.ResumeMode != EMissionResumeMode::FailOnLoad) continue;

			// Copy snapshots of this mission to FloorStateSnapshots (with uniqueness check)
			// Копируем снепшоты этой миссии в FloorStateSnapshots (с проверкой уникальности)
			for (auto& MissionPair : MissionFloorSnapshots)
			{
				const FInteriorFloorKey& FloorKey = MissionPair.Key;
				TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = MissionPair.Value;

				if (!PerFloor.Contains(MissionId)) continue;

				TArray<FFloorSavedActorState>& Snapshots = PerFloor[MissionId];
				TArray<FFloorSavedActorState>& FloorArray = FloorStateSnapshots.FindOrAdd(FloorKey);

				for (const FFloorSavedActorState& Snapshot : Snapshots)
				{
					if (ShouldUseActor(Envelope, FloorActorTypeToEnvelopeChannel(Snapshot.ActorType), EMissionEndReason::Failed, false))
					{
						// Update or add
						// Обновляем или добавляем
						bool bFound = false;
						for (int32 i = 0; i < FloorArray.Num(); ++i)
						{
							if (FloorArray[i].ItemId == Snapshot.ItemId)
							{
								FloorArray[i] = Snapshot;
								bFound = true;
								break;
							}
						}
						if (!bFound)
							FloorArray.Add(Snapshot);
					}
				}

				CopyMissionSpawnedToGlobal(MissionSpawnedActorsByInteriorFloor, SpawnedActorsByInteriorFloor, Envelope, EMissionEndReason::Failed, FloorKey, MissionId);
				CopyMissionDestroyedToGlobal(MissionDestroyedActorsByInteriorFloor, DestroyedActorsByInteriorFloor, Envelope, EMissionEndReason::Failed, FloorKey, MissionId);
			}

			// Remove mission snapshots from temporary storages
			// Удаляем снепшоты миссии из временных хранилищ
			RemoveMissionSnapshots(MissionFloorSnapshots, MissionId);
			RemoveMissionFromPopulationMap(MissionSpawnedActorsByInteriorFloor, MissionId);
			RemoveMissionFromPopulationMap(MissionDestroyedActorsByInteriorFloor, MissionId);
		}

		// --- Remove missions with FailOnLoad from ActiveMissions ---
		// --- Удаление миссий с FailOnLoad из ActiveMissions ---
		TArray<FName> MissionsToRemove;
		for (const auto& Pair : ActiveMissions)
		{
			UMissionController* Ctrl = Pair.Value.Controller;
			if (!Ctrl) continue;
			UMissionAsset* Asset = Ctrl->GetMissionAsset();
			if (!Asset) continue;
			FMissionEnvelope Envelope = Asset->Envelopes[Pair.Value.MissionStep];
			if (Envelope.ResumeMode == EMissionResumeMode::FailOnLoad)
			{
				MissionsToRemove.Add(Pair.Key);
			}
		}
		for (const FName& MissionId : MissionsToRemove)
		{
			ActiveMissions.Remove(MissionId);
		}
	}

	IsLoadComplete = true;
}

UMissionController* UInteriorSubsystem::CreateMission(UMissionAsset* MissionAsset)
{
	if (!MissionAsset)
	{
		return nullptr;
	}

	const FName MissionId = MissionAsset->GetMissionId();

	if (ActiveMissions.Contains(MissionId))
	{
		return ActiveMissions[MissionId].Controller;
	}
	// Create a controller of the required class (from the asset). If a Blueprint class is specified in the asset,
	// it will be instantiated and Activate()/OnMissionActivated() calls will go to the BP implementation.
	// Создаём контроллер нужного класса (из ассета). Если в ассете задан Blueprint класс,
	// он будет инстанцирован и вызовы Activate()/OnMissionActivated будут попадать в BP-реализацию.
	UMissionController* Controller = nullptr;
	if (MissionAsset->ControllerClass && MissionAsset->ControllerClass->IsChildOf(UMissionController::StaticClass()))
	{
		Controller = NewObject<UMissionController>(GetGameInstance(), MissionAsset->ControllerClass);
	}
	else
	{
		// Fallback to base C++ controller
		// Фоллбек на базовый C++ контроллер
		Controller = NewObject<UMissionController>(GetGameInstance());
		if (!MissionAsset->ControllerClass)
		{
			UE_LOG(LogTemp, Verbose, TEXT("MissionSubsystem::CreateMission: ControllerClass not set in MissionAsset '%s', using default UMissionController"), *MissionId.ToString());
			return nullptr;
		}
	}

	Controller->InitFromAsset(MissionAsset, GetGameInstance());

	FActiveMissionInterior Entry;
	Entry.MissionId = MissionId;
	Entry.Controller = Controller;
	ActiveMissions.Add(MissionId, Entry);

	return Controller;
}

void UInteriorSubsystem::HandleReleaseMissionSnapshot(const FOutcomeEventBase& Outcome)
{
	if (UReleaseMissionSnapshotPayload* P = Cast<UReleaseMissionSnapshotPayload>(Outcome.Payload))
	{
		ReleaseMissionSnapshot(P->MissionId, P->Envelope, P->Policy, P->bIsCompletion);
	}
}

void UInteriorSubsystem::HandleUpdateMissionList(const FOutcomeEventBase& Outcome)
{
	if (UUpdateMissionListPayload* P = Cast<UUpdateMissionListPayload>(Outcome.Payload))
	{
		// Determine the current Envelope for saving snapshot (if required)
		// Определяем текущий Envelope для сохранения snapshot (если требуется)
		FMissionEnvelope Envelope;
		if (P->IsUpdateSnapshots)
		{
			auto ActiveMissionInterior = ActiveMissions.Find(P->CurrentMissionId);

			if (ActiveMissionInterior)
			{
				int32 MissionStep = ActiveMissionInterior->MissionStep;
				auto Controller = ActiveMissionInterior->Controller;
				TArray<FMissionEnvelope> Envelopes = Controller->GetEnvelopes();
				if (Envelopes.IsValidIndex(MissionStep))
				{
					Envelope = Envelopes[MissionStep];
				}
			}
		}


		if (P->IsUpdateSnapshots)
		{
			StoreCurrentLevelComplete(Envelope, P->CurrentMissionId, P->Reason);
		}

		// Update the list of active missions
		// Обновляем список активных миссий
		ActiveMissions.Empty();
		TArray<FName> Keys;
		TMap<FName, FActiveMissionEntry> Map = P->ActiveMissions;
		Map.GetKeys(Keys);

		for (const FName& MissionId : Keys)
		{
			if (!MissionId.IsNone())
			{
				FActiveMissionInterior MissionInterior;
				MissionInterior.MissionId = MissionId;
				MissionInterior.MissionStep = Map[MissionId].MissionStep;
				MissionInterior.Controller = Map[MissionId].Controller;
				MissionInterior.Controller->MissionStep = MissionInterior.MissionStep;
				ActiveMissions.Add(MissionId, MissionInterior);
			}
		}
	}
}

void UInteriorSubsystem::HandleCompleteMission(const FOutcomeEventBase& Outcome)
{
	if (UApplyMissionCompletionPolicyPayload* P = Cast<UApplyMissionCompletionPolicyPayload>(Outcome.Payload))
	{
		FName MissionId = P->MissionId;
		if (MissionId.IsNone()) return;

		// When completing a mission — apply the policy to saved snapshots for this mission
		// При завершении миссии — применить политику к сохранённым snapshot-ам для этой миссии
		auto MissionInterior = ActiveMissions.Find(MissionId);

		if (MissionInterior)
		{
			TObjectPtr<UMissionController> Controller = MissionInterior->Controller;
			UMissionAsset* MissionAsset = Controller->GetMissionAsset();
			FMissionEnvelope Envelope = MissionAsset->Envelopes[MissionInterior->MissionStep];

			StoreCurrentLevelComplete(Envelope, MissionId, P->EndReason);

			ActiveMissions.Remove(MissionId);
			MissionFloorSnapshots.Find(CurrentKey)->Remove(MissionId);
		}
		RemoveMissionSnapshots(MissionFloorSnapshots, MissionId);
		RemoveMissionFromPopulationMap(MissionSpawnedActorsByInteriorFloor, MissionId);
		RemoveMissionFromPopulationMap(MissionDestroyedActorsByInteriorFloor, MissionId);
	}
}

void UInteriorSubsystem::StoreCurrentLevel(FMissionEnvelope Envelope, FName MissionId, EMissionEndReason EndReason)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FMissionEnvelopeScope Scope = Envelope.Scope;

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);

	for (const auto& S : Scope.InteriorScopes)
	{
		if (!S)
		{
			continue;
		}

		//ScopesFound++;
		FGuid FloorGuid = S->FloorID;
		FGuid InteriorSetID = S->InteriorSetID;
		if (!InteriorSetID.IsValid())
		{
			UInteriorSetAsset* LoadedSet = S->ParentInteriorSet.LoadSynchronous();
			if (LoadedSet) InteriorSetID = LoadedSet->InteriorSetID;
			else
			{
				continue;
			}
		}

		FString ScopeLevelName = S->FloorLevel.ToSoftObjectPath().GetLongPackageName();

		FString NormTarget = NormalizeLevelName(ScopeLevelName);

		// Also normalize the current level name (in case it has a prefix)
		// Также нормализуем текущее имя уровня (на случай если оно с префиксом)
		FString NormCurrent = NormalizeLevelName(CurrentLevelName);

		if (NormTarget == NormCurrent)
		{
			SaveFloorActorsState(InteriorSetID, FloorGuid, MissionId, EndReason);
		}
	}
}

void UInteriorSubsystem::StoreCurrentLevelComplete(FMissionEnvelope Envelope, FName MissionId, EMissionEndReason EndReason)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FMissionEnvelopeScope Scope = Envelope.Scope;

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);

	for (const auto& S : Scope.InteriorScopes)
	{
		if (!S)
		{
			continue;
		}

		//ScopesFound++;
		FGuid FloorGuid = S->FloorID;
		FGuid InteriorSetID = S->InteriorSetID;
		if (!InteriorSetID.IsValid())
		{
			UInteriorSetAsset* LoadedSet = S->ParentInteriorSet.LoadSynchronous();
			if (LoadedSet) InteriorSetID = LoadedSet->InteriorSetID;
			else
			{
				continue;
			}
		}

		FString ScopeLevelName = S->FloorLevel.ToSoftObjectPath().GetLongPackageName();

		FString NormTarget = NormalizeLevelName(ScopeLevelName);

		// Also normalize the current level name (in case it has a prefix)
		// Также нормализуем текущее имя уровня (на случай если оно с префиксом)
		FString NormCurrent = NormalizeLevelName(CurrentLevelName);

		if (NormTarget == NormCurrent)
		{
			SaveFloorActorsStateComplete(InteriorSetID, FloorGuid, MissionId, EndReason);
		}
	}
}

void UInteriorSubsystem::ApplySpawnGroupPolicy(const FSpawnGroupId& GroupId, EChannelPolicy Policy, EMissionEndReason Reason)
{

}

void UInteriorSubsystem::ReleaseMissionSnapshot(FName MissionId, const FMissionEnvelope& Envelope, EJobSpacePolicy Policy, bool bIsCompletion /*= false*/)
{
	if (MissionId.IsNone()) return;

	UWorld* World = GetWorld();

	// If this is simply leaving a floor during a mission — do nothing extra:
	// Если это просто покидание этажа во время миссии — ничего дополнительно не делать:
	if (!bIsCompletion)
	{
		return;
	}

	// Handle mission completion (bIsCompletion == true)
	// Обработка завершения миссии (bIsCompletion == true)
	switch (Policy)
	{
	case EJobSpacePolicy::Reset:
	{
		// ResetAll: restore from base FloorStateSnapshots (if any)
		// ResetAll: восстановить из базового FloorStateSnapshots (если есть)
		for (auto& Pair : MissionFloorSnapshots)
		{
			const FInteriorFloorKey& FloorKey = Pair.Key;
			TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = Pair.Value;
			if (!PerFloor.Contains(MissionId)) continue;

			if (const TArray<FFloorSavedActorState>* BaseSnap = FloorStateSnapshots.Find(FloorKey))
			{
				if (World && BaseSnap && !BaseSnap->IsEmpty())
				{
					RestoreFromSnapshotArray(World, *BaseSnap, FloorKey.InteriorSetId, FloorKey.FloorId, Envelope);
				}
			}
		}
		break;
	}

	case EJobSpacePolicy::Freeze:
	{
		// FreezeAll: copy mission snapshot to permanent storage FloorStateSnapshots
		// FreezeAll: копируем mission snapshot в постоянное хранилище FloorStateSnapshots
		for (auto& Pair : MissionFloorSnapshots)
		{
			const FInteriorFloorKey& FloorKey = Pair.Key;
			TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = Pair.Value;
			if (const TArray<FFloorSavedActorState>* MissionSnap = PerFloor.Find(MissionId))
			{
				FloorStateSnapshots.FindOrAdd(FloorKey) = *MissionSnap;
			}
		}
		break;
	}

	case EJobSpacePolicy::Partial:
	{
		// Partial: apply policy per channel
		// Partial: применяем политику поканально
		for (auto& Pair : MissionFloorSnapshots)
		{
			const FInteriorFloorKey& FloorKey = Pair.Key;
			TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = Pair.Value;
			const TArray<FFloorSavedActorState>* MissionSnap = PerFloor.Find(MissionId);
			if (!MissionSnap) continue;

			for (const FEnvelopeChannelEntry& ChannelEntry : Envelope.RuntimePolicyChannels)
			{
				if (ChannelEntry.Policy == EChannelPolicy::Freeze)
				{
					TArray<FFloorSavedActorState>& BaseSnap = FloorStateSnapshots.FindOrAdd(FloorKey);

					for (const FFloorSavedActorState& ActorSnap : *MissionSnap)
					{
						bool bMatchChannel = false;
						if (World)
						{
							for (TActorIterator<AActor> It(World); It; ++It)
							{
								AActor* Actor = *It;
								if (!IsValid(Actor)) continue;
								UFloorAssignmentComponent* FAC = Actor->FindComponentByClass<UFloorAssignmentComponent>();
								if (!FAC) continue;
								if (FAC->ItemId != ActorSnap.ItemId) continue;
								if (FAC->SnapshotChannel == static_cast<ESnapshotChannel>(ChannelEntry.Channel))
								{
									bMatchChannel = true;
								}
								break;
							}
						}
						if (!bMatchChannel) continue;

						bool bUpdated = false;
						for (FFloorSavedActorState& Existing : BaseSnap)
						{
							if (Existing.ItemId == ActorSnap.ItemId)
							{
								Existing = ActorSnap;
								bUpdated = true;
								break;
							}
						}
						if (!bUpdated) BaseSnap.Add(ActorSnap);
					}
				}
				else if (ChannelEntry.Policy == EChannelPolicy::Reset)
				{
					if (const TArray<FFloorSavedActorState>* BaseSnap = FloorStateSnapshots.Find(FloorKey))
					{
						if (World && BaseSnap && !BaseSnap->IsEmpty())
						{
							RestoreFromSnapshotArray(World, *BaseSnap, FloorKey.InteriorSetId, FloorKey.FloorId, Envelope);
						}
					}
				}
			}
		}
		break;
	}

	default:
		break;
	}

	// Clear mission snapshots for this mission
	// Очищаем mission snapshots для этой миссии
	for (auto It = MissionFloorSnapshots.CreateIterator(); It; ++It)
	{
		TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = It->Value;
		PerFloor.Remove(MissionId);
		if (PerFloor.Num() == 0)
			It.RemoveCurrent();
	}
}

void UInteriorSubsystem::OnActorSpawned(AActor* SpawnedActor)
{
	if (!IsValid(SpawnedActor)) return;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UInteriorSubsystem::TryRegisterActor, SpawnedActor, 20));
	}
}

void UInteriorSubsystem::TryRegisterActor(AActor* SpawnedActor, int32 AttemptsLeft)
{
	if (!IsValid(SpawnedActor)) return;

	UFloorAssignmentComponent* Comp = SpawnedActor->FindComponentByClass<UFloorAssignmentComponent>();
	if (!Comp || Comp->SnapshotChannel == ESnapshotChannel::None) return;

	// If an actor with this ItemId is already registered in SpawnedActorsByInteriorFloor, skip registration
	// Если актор с таким ItemId уже зарегистрирован в SpawnedActorsByInteriorFloor, пропускаем регистрацию
	if (ContainsActorIdInSpawned(SpawnedActorsByInteriorFloor, Comp->ItemId))
	{
		return;
	}

	if (Comp->ItemId.IsValid())
	{
		// publish event / публикуем событие
		if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
		{
			UFloorPlacementPayload* Payload = EventBus->CreatePayload<UFloorPlacementPayload>();
			if (Payload)
			{
				UClass* ActorClass = SpawnedActor->GetClass();
				Payload->Setup(Comp->ActorType, SpawnedActor->GetActorTransform(), Comp->ItemId, ActorClass);
				FOutcomeEventBase Ev;
				Ev.OutcomeType = EOutcomeType::Interior;
				Ev.OutcomeInterior = EOutcomeInterior::FloorPlacementRegistered;
				Ev.Payload = Payload;
				EventBus->PublishOutcome(Ev);
			}
		}
		return;
	}

	if (AttemptsLeft > 0)
	{
		if (UWorld* World = GetWorld())
		{
			FTimerHandle RetryHandle;
			World->GetTimerManager().SetTimer(RetryHandle, FTimerDelegate::CreateUObject(this, &UInteriorSubsystem::TryRegisterActor, SpawnedActor, AttemptsLeft - 1), 0.05f, false);
		}
	}
}

void UInteriorSubsystem::RemoveRecordsForChannel(TArray<FFloorPopulationRecord>& Records, EEnvelopeChannel Channel)
{
	RemoveRecordsForChannelHelper(Records, Channel);
}

AActor* UInteriorSubsystem::FindActorByItemId(const FGuid& ItemId) const
{
	UWorld* World = GetWorld();
	if (!World) return nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;
		if (UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>())
		{
			if (Comp->ItemId == ItemId)
				return Actor;
		}
	}
	return nullptr;
}

void UInteriorSubsystem::SpawnRecords(const TArray<FFloorPopulationRecord>& Records)
{
	for (const FFloorPopulationRecord& Rec : Records)
	{
		if (!Rec.SourceClass) continue;
		if (FindActorByItemId(Rec.ActorId)) continue; // do not spawn again / не спавним повторно
		FActorSpawnParameters Params;
		AActor* Spawned = GetWorld()->SpawnActor<AActor>(Rec.SourceClass, Rec.WorldTransform, Params);
		if (!Spawned) continue;
		if (UFloorAssignmentComponent* Comp = Spawned->FindComponentByClass<UFloorAssignmentComponent>())
		{
			Comp->ItemId = Rec.ActorId;
			Comp->SnapshotChannel = ESnapshotChannel::Snapshot;
			Comp->ActorType = Rec.ActorType;
		}
	}
}

void UInteriorSubsystem::DestroyRecords(const TArray<FFloorPopulationRecord>& Records)
{
	for (const FFloorPopulationRecord& Rec : Records)
	{
		AActor* Actor = FindActorByItemId(Rec.ActorId);
		if (Actor && IsValid(Actor))
			Actor->Destroy();
	}
}