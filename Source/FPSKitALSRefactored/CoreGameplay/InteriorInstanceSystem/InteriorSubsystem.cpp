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

// -----------------------------------------------------------------------------
// Helper functions for persistent population (channel filtering)
// -----------------------------------------------------------------------------

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

static void RemoveRecordsForChannelHelper(TArray<FFloorPopulationRecord>& Records, EEnvelopeChannel Channel)
{
	Records.RemoveAll([Channel](const FFloorPopulationRecord& Rec) {
		return FloorActorTypeToEnvelopeChannel(Rec.ActorType) == Channel;
		});
}

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

auto RemoveRecordByItemId = [](TMap<FInteriorFloorKey, FFloorPopulationBuckets>& Map, const FGuid& ItemId) -> bool
	{
		if (!ItemId.IsValid()) return false;
		bool bRemoved = false;

		// Проходим по всем этажам
		for (auto& Pair : Map)
		{
			FFloorPopulationBuckets& Buckets = Pair.Value;

			// Лямбда для удаления из массива
			auto RemoveFromArray = [&](TArray<FFloorPopulationRecord>& Arr)
				{
					int32 OldCount = Arr.Num();
					Arr.RemoveAll([&ItemId](const FFloorPopulationRecord& Rec) { return Rec.ActorId == ItemId; });
					if (OldCount != Arr.Num()) bRemoved = true;
				};

			RemoveFromArray(Buckets.HeavyFurniture);
			RemoveFromArray(Buckets.LightItems);
			RemoveFromArray(Buckets.Terminals);
			RemoveFromArray(Buckets.NPCSpawners);
			RemoveFromArray(Buckets.Debris);
		}

		return bRemoved;
	};
// -----------------------------------------------------------------------------
// JSON serialization helpers for snapshot structures
// -----------------------------------------------------------------------------

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

static TSharedPtr<FJsonObject> PropertyEntryToJson(const FFloorSavedPropertyEntry& Entry)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("PropertyName"), Entry.PropertyName.ToString());
	Obj->SetStringField(TEXT("ValueText"), Entry.ValueText);
	return Obj;
}

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

static TSharedPtr<FJsonObject> ActorStateToJson(const FFloorSavedActorState& State)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("ItemId"), State.ItemId.ToString());
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

static FFloorSavedActorState ActorStateFromJson(const TSharedPtr<FJsonObject>& Obj)
{
	FFloorSavedActorState State;
	if (!Obj.IsValid()) return State;
	FGuid::Parse(Obj->GetStringField(TEXT("ItemId")), State.ItemId);
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
// -----------------------------------------------------------------------------
namespace
{
	// Нормализация: берём базовое имя файла без пути, убираем PIE-префиксы и приводим к нижнему регистру
	auto NormalizeLevelName = [](const FString& InPath) -> FString
		{
			if (InPath.IsEmpty()) return FString();
			FString PackagePath = InPath;
			if (PackagePath.Contains(TEXT(".")))
				PackagePath = FPackageName::ObjectPathToPackageName(PackagePath);
			FString Base = FPaths::GetBaseFilename(PackagePath);
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

	static void CollectSaveGameProperties(UObject* Obj, TArray<FFloorSavedPropertyEntry>& OutProps)
	{
		if (!IsValid(Obj)) return;
		for (TFieldIterator<FProperty> It(Obj->GetClass()); It; ++It)
		{
			FProperty* Prop = *It;
			if (!Prop->HasAllPropertyFlags(CPF_SaveGame)) continue;

			FString Exported;
			Prop->ExportText_InContainer(0, Exported, Obj, nullptr, Obj, PPF_None);
			OutProps.Add({ Prop->GetFName(), MoveTemp(Exported) });
		}
	}

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

	static void SnapshotActor(AActor* Actor, FFloorSavedActorState& OutSnapshot)
	{
		if (!IsValid(Actor)) return;

		UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
		if (!Comp) return;

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

	static void RestoreActorSnapshot(AActor* Actor, const FFloorSavedActorState& Snapshot, bool bApplyWorldTransform = true)
	{
		if (!IsValid(Actor)) return;

		if (bApplyWorldTransform)
			Actor->SetActorTransform(Snapshot.ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);

		ApplySaveGameProperties(Actor, Snapshot.ActorProperties);

		TMap<FName, UActorComponent*> ComponentsByName;
		for (UActorComponent* C : Actor->GetComponents())
		{
			if (!IsValid(C)) continue;
			ComponentsByName.Add(C->GetFName(), C);
		}

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

		for (const FFloorSavedComponentState& CState : Snapshot.ComponentStates)
		{
			UActorComponent** Found = ComponentsByName.Find(CState.ComponentName);
			if (!Found || !IsValid(*Found)) continue;
			UActorComponent* Target = *Found;

			ApplySaveGameProperties(Target, CState.Properties);

			if (Target->IsActive() != CState.bWasActive)
				CState.bWasActive ? Target->Activate(true) : Target->Deactivate();

			if (USceneComponent* SC = Cast<USceneComponent>(Target))
				RestoreSceneComponentTransform(SC, CState);
		}
	}
} // namespace
// -----------------------------------------------------------------------------
// JSON serialization helpers for population structures
// -----------------------------------------------------------------------------

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
	case EFloorActorType::NPC_Spawner:    return TEXT("NPC_Spawner");
	case EFloorActorType::LocationTriggers: return TEXT("LocationTriggers");
	default: return TEXT("LightItem");
	}
}

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
	if (Str == TEXT("NPC_Spawner"))    return EFloorActorType::NPC_Spawner;
	if (Str == TEXT("LocationTriggers"))return EFloorActorType::LocationTriggers;
	return EFloorActorType::LightItem;
}

static TSharedPtr<FJsonObject> PopulationRecordToJson(const FFloorPopulationRecord& Record)
{
	TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("ActorType"), ActorTypeToString(Record.ActorType));
	if (Record.SourceClass)
		Obj->SetStringField(TEXT("SourceClass"), Record.SourceClass->GetPathName());
	Obj->SetStringField(TEXT("ActorId"), Record.ActorId.ToString());
	Obj->SetBoolField(TEXT("IsNewObject"), Record.IsNewObject);           // <-- добавлено
	Obj->SetObjectField(TEXT("WorldTransform"), TransformToJsonObject(Record.WorldTransform));
	Obj->SetBoolField(TEXT("bHasAnchor"), Record.bHasAnchor);
	return Obj;
}

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

	// Чтение поля IsNewObject, если его нет в сохранении – по умолчанию true
	if (!Obj->TryGetBoolField(TEXT("IsNewObject"), Record.IsNewObject))
		Record.IsNewObject = true;   // <-- добавлено с fallback

	if (Obj->HasField(TEXT("WorldTransform")))
		Record.WorldTransform = TransformFromJsonObject(Obj->GetObjectField(TEXT("WorldTransform")));
	Obj->TryGetBoolField(TEXT("bHasAnchor"), Record.bHasAnchor);
	return Record;
}

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

// -----------------------------------------------------------------------------
// SaveFloorActorsState
// -----------------------------------------------------------------------------

static bool ShouldSkipActor(const FMissionEnvelope& Envelope, EEnvelopeChannel ActorChannel, EMissionEndReason Reason, bool IsRead)
{
	// ищем запись для канала
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

	if (IsRead  || Reason != EMissionEndReason::None)
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

void UInteriorSubsystem::SaveFloorActorsState(const FGuid& InteriorSetId, const FGuid& FloorId, FName MissionId, EMissionEndReason Reason/*, EJobSpacePolicy JobSpacePolicy, const TArray<FEnvelopeChannelEntry>& Channels*/)
{
    UWorld* W = GetWorld();
    if (!W) return;

	UE_LOG(LogTemp, Warning, TEXT("[SAVE] SaveFloorActorsState: Mission=%s, Floor=%s/%s, Reason=%d"),
		*MissionId.ToString(), *InteriorSetId.ToString(), *FloorId.ToString(), (int32)Reason);

    FInteriorFloorKey Key(InteriorSetId, FloorId);

	auto MissionInterior = ActiveMissions.Find(MissionId);
	FMissionEnvelope Envelope = FMissionEnvelope();
	if (MissionInterior)
	{
		int32 MissionStep = MissionInterior->MissionStep;
		auto Controller = MissionInterior->Controller;
		TArray<FMissionEnvelope> Envelopes = Controller->GetEnvelopes();
		if(Envelopes.IsValidIndex(MissionStep))
		{
			Envelope = Envelopes[MissionStep];
		}
	}
	else
	{
		return;
	}
    // существующая логика: сохраняем в MissionFloorSnapshots[Key][MissionId]
	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: SaveFloorActorsState MissionFloorSnapshots count before: %d"), MissionFloorSnapshots.Num());
    TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = MissionFloorSnapshots.FindOrAdd(Key);
    TArray<FFloorSavedActorState>& Bucket = PerFloor.FindOrAdd(MissionId);

    TMap<FGuid, int32> ExistingIndex;
    for (int32 i = 0; i < Bucket.Num(); ++i)
        ExistingIndex.Add(Bucket[i].ItemId, i);

    int32 AddedCount   = 0;
    int32 UpdatedCount = 0;

    for (TActorIterator<AActor> It(W); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor)) continue;

        UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
        if (!Comp) continue;
        if (Comp->SnapshotChannel == ESnapshotChannel::None) continue;

		const EEnvelopeChannel ActorChannel = FloorActorTypeToEnvelopeChannel(Comp->ActorType);

		if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(Comp->ActorType), Reason, false))
			continue;

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
            if (ChildFloorComp->GetInteriorSetId() != InteriorSetId) continue;

			if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(Comp->ActorType), Reason, false))
				continue;

            FFloorSavedActorState ChildSnapshot;
            ChildSnapshot.ItemId              = ChildFloorComp->ItemId;
            ChildSnapshot.RelativeTransform   = ChildComp->GetRelativeTransform();
            ChildSnapshot.bHasRelativeTransform = true;
            SnapshotActor(ChildActor, ChildSnapshot);

            if (int32* ChildIdx = ExistingIndex.Find(ChildSnapshot.ItemId))
            {
                Bucket[*ChildIdx] = MoveTemp(ChildSnapshot);
                ++UpdatedCount;
            }
            else
            {
                Bucket.Add(MoveTemp(ChildSnapshot));
                ExistingIndex.Add(Bucket.Last().ItemId, Bucket.Num() - 1);
                ++AddedCount;
            }
        }

		if (Comp->SnapshotChannel != ESnapshotChannel::None)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SAVE]   Capturing actor: %s, ItemId=%s, Type=%d, Channel=%d"),
				*Actor->GetName(), *Comp->ItemId.ToString(), (int32)Comp->ActorType, (int32)Comp->SnapshotChannel);
		}
    }

    UE_LOG(LogTemp, Log,
        TEXT("InteriorSubsystem: SaveFloorActorsState — added %d, updated %d for mission '%s' floor %s/%s (total: %d)"),
        AddedCount, UpdatedCount, *MissionId.ToString(),
        *InteriorSetId.ToString(), *FloorId.ToString(), Bucket.Num());

	UE_LOG(LogTemp, Warning, TEXT("[SAVE] Finished: Added=%d, Updated=%d, Total bucket size=%d for Mission=%s"),
		AddedCount, UpdatedCount, Bucket.Num(), *MissionId.ToString());
}

// -----------------------------------------------------------------------------
// RestoreFloorActorsState
// -----------------------------------------------------------------------------

int32 UInteriorSubsystem::RestoreFloorActorsState(const FGuid& InteriorSetId, int32 CurrentMissionStep, const FGuid& FloorId)
{
    UWorld* W = GetWorld();
    if (!W) return 0;

    FInteriorFloorKey Key(InteriorSetId, FloorId);

    // Ищем все снимки миссий для данного этажа
	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: RestoreFloorActorsState MissionFloorSnapshots count before: %d"), MissionFloorSnapshots.Num());
    const TMap<FName, TArray<FFloorSavedActorState>>* PerFloor = MissionFloorSnapshots.Find(Key);
    if (!PerFloor || PerFloor->Num() == 0) return 0;

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

    int32 TotalRestored = 0;

    // Восстанавливаем снимки для каждой миссии, привязанной к этому этажу
    for (const auto& MissionPair : *PerFloor)
    {
        const TArray<FFloorSavedActorState>& Snapshots = MissionPair.Value;

        // First pass: независимые (root) акторы
        for (const FFloorSavedActorState& Snapshot : Snapshots)
        {
            if (Snapshot.bHasRelativeTransform) continue;
            AActor** ActorPtr = ActorByItemId.Find(Snapshot.ItemId);
            if (!ActorPtr || !IsValid(*ActorPtr)) continue;
            RestoreActorSnapshot(*ActorPtr, Snapshot, true);
            ++TotalRestored;
        }

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
                        bAppliedRelative = true;
                        break;
                    }
                }
            }

            if (!bAppliedRelative)
                ChildActor->SetActorTransform(Snapshot.ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);

            RestoreActorSnapshot(ChildActor, Snapshot, false);
            ++TotalRestored;
        }

        UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: RestoreFloorActorsState — restored %d actors for mission '%s' floor %s/%s"),
            TotalRestored, *MissionPair.Key.ToString(), *InteriorSetId.ToString(), *FloorId.ToString());
    }

    return TotalRestored;
}

// --- добавлен новый хелпер: восстанавливает actors напрямую из переданного массива снимков ---
int32 UInteriorSubsystem::RestoreFromSnapshotArray(UWorld* W, const TArray<FFloorSavedActorState>& Snapshots, const FGuid& InteriorSetId, const FGuid& FloorId, const FMissionEnvelope Envelope)
{
	UE_LOG(LogTemp, Warning, TEXT("[RESTORE] RestoreFromSnapshotArray for floor %s/%s, snapshot count=%d"),
		*InteriorSetId.ToString(), *FloorId.ToString(), Snapshots.Num());

	if (!W)
	{
		UE_LOG(LogTemp, Error, TEXT("[RESTORE] No world!"));
		return 0;
	}

	if (Snapshots.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RESTORE] Snapshots array is empty, nothing to restore"));
		return 0;
	}

	// Собираем всех акторов текущего уровня, имеющих FloorAssignmentComponent
	TMap<FGuid, AActor*> ActorByItemId;
	TArray<AActor*> AllActors;
	UGameplayStatics::GetAllActorsOfClass(W, AActor::StaticClass(), AllActors);
	for (AActor* Actor : AllActors)
	{
		if (!IsValid(Actor)) continue;
		if (UFloorAssignmentComponent* C = Actor->FindComponentByClass<UFloorAssignmentComponent>())
		{
			if (C->SnapshotChannel != ESnapshotChannel::None)
			{
				if (IsCurrentWorldMissionConst(Envelope))
				{
					if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(C->ActorType), EMissionEndReason::None, true))
					{
						RemoveRecordByItemId(SpawnedActorsByInteriorFloor, C->ItemId);
						RemoveRecordByItemId(DestroyedActorsByInteriorFloor, C->ItemId);
						continue;
					}
				}
				
				// Проверяем, не помечен ли актор на удаление через DestroyedActorsByInteriorFloor
				if (ContainsActorIdInSpawnedNew(DestroyedActorsByInteriorFloor, C->ItemId))
				{
					Actor->Destroy();
					UE_LOG(LogTemp, Warning, TEXT("[RESTORE]   Actor %s (ItemId=%s) marked as destroyed, removing"), *Actor->GetName(), *C->ItemId.ToString());
					continue;
				}

				FInteriorFloorKey Key(InteriorSetId, FloorId);
				TArray<FFloorSavedActorState>* ActorStates = FloorStateSnapshots.Find(Key);
				
				auto HasItemId = [](const TArray<FFloorSavedActorState>* ActorStates, const FGuid& ItemId) -> bool
					{
						return ActorStates && ActorStates->ContainsByPredicate([&ItemId](const FFloorSavedActorState& State)
							{
								return State.ItemId == ItemId;
							});
					};

				if (!IsCurrentWorldMissionConst(Envelope) /* && !IsLoadingFromSave*/)
				{
					if (HasItemId(ActorStates, C->ItemId))
					{
						ActorByItemId.Add(C->ItemId, Actor);
					}
				}
				else
				{
					ActorByItemId.Add(C->ItemId, Actor);
				}
			}
		}
		// Также проверяем дочерние акторы (ChildActorComponent)
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
				{
					if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(CC->ActorType), EMissionEndReason::None, true))
					{
						RemoveRecordByItemId(SpawnedActorsByInteriorFloor, CC->ItemId);
						RemoveRecordByItemId(DestroyedActorsByInteriorFloor, CC->ItemId);
						continue;
					}

					if (ContainsActorIdInSpawnedNew(DestroyedActorsByInteriorFloor, CC->ItemId))
					{
						ChildActor->Destroy();
						continue;
					}

					ActorByItemId.Add(CC->ItemId, ChildActor);
				}
			}
		}
	}

	int32 Restored = 0;

	// First pass: root actors (без относительного преобразования)
	for (const FFloorSavedActorState& Snapshot : Snapshots)
	{
		if (Snapshot.bHasRelativeTransform) continue;
		if (AActor** ActorPtr = ActorByItemId.Find(Snapshot.ItemId))
		{
			AActor* Actor = *ActorPtr;
			if (IsValid(Actor))
			{
				RestoreActorSnapshot(Actor, Snapshot, true);
				++Restored;
				UE_LOG(LogTemp, Warning, TEXT("[RESTORE]   Restored root actor %s (ItemId=%s)"), *Actor->GetName(), *Snapshot.ItemId.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[RESTORE]   Actor with ItemId=%s found but invalid"), *Snapshot.ItemId.ToString());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[RESTORE]   Root actor with ItemId=%s NOT FOUND in world"), *Snapshot.ItemId.ToString());
		}
	}

	// Second pass: child actors (с относительным преобразованием)
	for (const FFloorSavedActorState& Snapshot : Snapshots)
	{
		if (!Snapshot.bHasRelativeTransform) continue;
		if (AActor** ActorPtr = ActorByItemId.Find(Snapshot.ItemId))
		{
			AActor* ChildActor = *ActorPtr;
			if (!IsValid(ChildActor)) continue;

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
						ParentChildComp->SetRelativeTransform(Snapshot.RelativeTransform);
						bAppliedRelative = true;
						break;
					}
				}
			}
			if (!bAppliedRelative)
				ChildActor->SetActorTransform(Snapshot.ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);

			RestoreActorSnapshot(ChildActor, Snapshot, false);
			++Restored;
			UE_LOG(LogTemp, Warning, TEXT("[RESTORE]   Restored child actor %s (ItemId=%s)"), *ChildActor->GetName(), *Snapshot.ItemId.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[RESTORE]   Child actor with ItemId=%s NOT FOUND in world"), *Snapshot.ItemId.ToString());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[RESTORE] Finished: restored %d actors out of %d snapshots"), Restored, Snapshots.Num());
	return Restored;
}

// -----------------------------------------------------------------------------
// EventBus handlers — Registration
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
		R.ItemId           = P->ItemId;
		R.OwnerActor       = Owner;
		R.InteractionRange = P->InteractionRange;
		R.SubsystemType    = P->SubsystemType;
		RegisteredItems.Add(P->ItemId, R);

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Registered item '%s' (range=%.1f)"), *OwnerName, P->InteractionRange);

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

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Unregistered item '%s'"), *OwnerName);

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

	// Используем текущий этаж (CurrentKey), на котором произошёл спавн/удаление
	// (в вашем коде ранее использовался CurrentKey, а не переданный P->InteriorSetId/P->FloorId)
	if (Outcome.OutcomeInterior == EOutcomeInterior::FloorPlacementRegistered)
	{
		bool IsMissionWorld = false;

		FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
		FString MissionName;
		for (const auto Pair : ActiveMissions)
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
					break;
				}
			}

			if (IsMissionWorld)
			{
				break;
			}
		}


		// Добавляем запись в SpawnedActorsByInteriorFloor
		FFloorPopulationRecord NewRecord;
		NewRecord.ActorType = P->ActorType;
		NewRecord.WorldTransform = P->WorldTransform;
		NewRecord.ActorId = P->ItemId;
		NewRecord.bHasAnchor = P->AnchorId.IsValid();
		NewRecord.SourceClass = P->ActorClass;

		FFloorPopulationBuckets& BucketsAdded = SpawnedActorsByInteriorFloor.FindOrAdd(CurrentKey);

		FMissionEnvelope Envelope = FMissionEnvelope();
		if (!IsCurrentWorldMission(Envelope)) return;

		switch (P->ActorType)
		{
		case EFloorActorType::HeavyFurniture: BucketsAdded.HeavyFurniture.Add(NewRecord); break;
		case EFloorActorType::LightItem:      BucketsAdded.LightItems.Add(NewRecord);      break;
		case EFloorActorType::Terminal:       BucketsAdded.Terminals.Add(NewRecord);       break;
		case EFloorActorType::NPC_Spawner:    BucketsAdded.NPCSpawners.Add(NewRecord);     break;
		case EFloorActorType::Debris:         BucketsAdded.Debris.Add(NewRecord);          break;
		default:                              BucketsAdded.LightItems.Add(NewRecord);      break;
		}

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Placement spawned for floor %s/%s (ActorId=%s)"),
			*CurrentKey.InteriorSetId.ToString(), *CurrentKey.FloorId.ToString(), *P->ItemId.ToString());

		UE_LOG(LogTemp, Warning, TEXT("[DYNAMIC] SPAWN registered: ItemId=%s, Type=%d, Floor=%s/%s"),
			*P->ItemId.ToString(), (int32)P->ActorType,
			*CurrentKey.InteriorSetId.ToString(), *CurrentKey.FloorId.ToString());
	}
	else if (Outcome.OutcomeInterior == EOutcomeInterior::FloorPlacementUnregistered)
	{
		// Добавляем запись в DestroyedActorsByInteriorFloor
		FFloorPopulationBuckets& BucketsDestroyed = DestroyedActorsByInteriorFloor.FindOrAdd(CurrentKey);
		FFloorPopulationRecord NewRecord;
		NewRecord.ActorType = P->ActorType;
		NewRecord.WorldTransform = P->WorldTransform;
		NewRecord.ActorId = P->ItemId;
		NewRecord.bHasAnchor = P->AnchorId.IsValid();

		switch (P->ActorType)
		{
		case EFloorActorType::HeavyFurniture:
		{
			if (!BucketsDestroyed.HeavyFurniture.ContainsByPredicate([&NewRecord](const FFloorPopulationRecord& Existing) {
				return Existing.ActorId == NewRecord.ActorId;
				}))
			{
				BucketsDestroyed.HeavyFurniture.Add(NewRecord);
			}
			break;
		}

		case EFloorActorType::LightItem:
		{
			if (!BucketsDestroyed.LightItems.ContainsByPredicate([&NewRecord](const FFloorPopulationRecord& Existing) {
				return Existing.ActorId == NewRecord.ActorId;
				}))
			{
				BucketsDestroyed.LightItems.Add(NewRecord);
			}
			break;
		}

		case EFloorActorType::Terminal:
		{
			if (!BucketsDestroyed.Terminals.ContainsByPredicate([&NewRecord](const FFloorPopulationRecord& Existing) {
				return Existing.ActorId == NewRecord.ActorId;
				}))
			{
				BucketsDestroyed.Terminals.Add(NewRecord);
				break;
			}
		}

		case EFloorActorType::NPC_Spawner:
		{
			if (!BucketsDestroyed.NPCSpawners.ContainsByPredicate([&NewRecord](const FFloorPopulationRecord& Existing) {
				return Existing.ActorId == NewRecord.ActorId;
				}))
			{
				BucketsDestroyed.NPCSpawners.Add(NewRecord);
			}
			break;
		}

		case EFloorActorType::Debris:
		{
			if (!BucketsDestroyed.Debris.ContainsByPredicate([&NewRecord](const FFloorPopulationRecord& Existing) {
				return Existing.ActorId == NewRecord.ActorId;
				}))
			{
				BucketsDestroyed.Debris.Add(NewRecord);
			}
			break;
		}

		default: {}
		}

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Placement destroyed for floor %s/%s (ActorId=%s)"),
			*CurrentKey.InteriorSetId.ToString(), *CurrentKey.FloorId.ToString(), *P->ItemId.ToString());

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

		if (ContainsActorIdInSpawned(SpawnedActorsByInteriorFloor, P->ItemId))
		{
			RemoveFromSpawnedByActorId(DestroyedActorsByInteriorFloor, P->ItemId);
		}
		RemoveFromSpawnedByActorId(SpawnedActorsByInteriorFloor, P->ItemId);

		UE_LOG(LogTemp, Warning, TEXT("[DYNAMIC] DESTROY unregistered: ItemId=%s, Type=%d, Floor=%s/%s"),
			*P->ItemId.ToString(), (int32)P->ActorType,
			*CurrentKey.InteriorSetId.ToString(), *CurrentKey.FloorId.ToString());
	}
}

bool UInteriorSubsystem::IsCurrentWorldMission(FMissionEnvelope& Envelope)
{
	UWorld* World = GetWorld();
	if (!World) return false;

	bool IsMissionWorld = false;

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	for (const auto Pair : ActiveMissions)
	{
		const UMissionController* Ctrl = Pair.Value.Controller;
		if (!Ctrl) continue;

		const UMissionAsset* Asset = Ctrl->GetMissionAsset();
		if (!Asset) continue;

		const EMissionStatus Status = Ctrl->GetStatus();
		const EMissionEndReason EndReason = Ctrl->GetEndReason();
		Envelope = Asset->Envelopes[Pair.Value.MissionStep];
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
				break;
			}
		}

		if (IsMissionWorld)
		{
			break;
		}
	}

	return IsMissionWorld;
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
		if(CurrentMissionStep < 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem::HandleFloorStateRestore: invalid CurrentMissionStep=%d"), CurrentMissionStep);
			return;
		}

		if (P->MissionId != NAME_None)
		{
			RestoreMissionFloorState(P->MissionId, CurrentMissionStep,  FInteriorFloorKey(P->InteriorSetId, P->FloorId));
		}
		else
		{
			RestoreFloorActorsState(P->InteriorSetId, CurrentMissionStep, P->FloorId);
		}
	}
}

// -----------------------------------------------------------------------------
// HandleFloorTransition
// -----------------------------------------------------------------------------

void UInteriorSubsystem::HandleFloorTransition(const FOutcomeEventBase& Outcome)
{
	UWorld* World = GetWorld();
	if (!World) return;

	UInteriorTransitionPayload* P = Cast<UInteriorTransitionPayload>(Outcome.Payload);
	// Исправленная проверка: сначала проверяем P на валидность, потом условия
	if (!P || (P->DestinationLink.IsValid() && !P->IsUseAnchor) || (!P->DestinationLink.IsValid() && P->IsUseAnchor))
	{
		UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem::HandleFloorTransition: payload invalid"));
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

	// Получаем путь целевого уровня из payload
	const FString TargetLevelPath = P->IsUseAnchor ? P->GetTargetLevelPackageName() : P->TargetLevelPath;
	bool IsUseTravel = P->IsUseTravel;

	if (IsUseTravel)
	{
		if (TargetLevelPath.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem::HandleFloorTransition: failed to determine target level (не удалось определить целевой уровень)"));
			OnTransitionCompleted.Broadcast(false, FLocationAnchorLink(), false);
			return;
		}

		const FGuid TargetAnchorID = P->GetTargetAnchorID();

		// Получаем короткое имя текущего уровня через GameplayStatics — параметр true убирает PIE‑префиксы
		FString CurrentLevelShort = UGameplayStatics::GetCurrentLevelName(W, true);
		CurrentLevelShort = CurrentLevelShort.ToLower();

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

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem::HandleFloorTransition: currentShort='%s', targetPackage='%s' (norm='%s'), anchor=[%s]"),
			*CurrentLevelShort, *TargetLevelPath, *NormTarget, *TargetAnchorID.ToString());

		// Сравниваем короткое имя текущего уровня с нормализованным именем целевого уровня
		if (P->IsUseAnchor)
		{
			if (!CurrentLevelShort.IsEmpty() && NormTarget.IsEmpty() && CurrentLevelShort == NormTarget)
			{
				UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: same map (by name) — teleporting in-place (та же карта (по имени) — телепортируем на месте)"));
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

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: SeamlessTravel → '%s'"), *TargetLevelPath);
	}
	FTimerDelegate TravelDelegate = FTimerDelegate::CreateLambda([TargetLevelPath, this]()
	{
		if (UWorld* LocalW = GetWorld())
			LocalW->SeamlessTravel(TargetLevelPath, true);
	});

	// Уведомляем подписчиков: сейчас будет уход с исходного этажа
	// Сначала оповестим через EventBus, чтобы MissionSubsystem мог запросить сохранение mission-snapshots.

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get() ? CachedEventBus.Get() : GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		// Reuse existing payload P (contains SourceFloor info)
		FOutcomeEventBase Ev;
		Ev.OutcomeType = EOutcomeType::Interior;
		Ev.OutcomeInterior = EOutcomeInterior::FloorLeaving; // notify listeners that we're leaving this floor
		Ev.Payload = P;
		EventBus->PublishOutcome(Ev);
	}
	// Затем стандартный локальный broadcast
	OnFloorExiting.Broadcast(P->SourceFloor);

	if (IsUseTravel)
	{
		UnsubscribeFromSpawnActor();
		W->GetTimerManager().SetTimer(TravelTimerHandle, TravelDelegate, 1.0f, false);
	}
}

// -----------------------------------------------------------------------------
// TeleportToAnchor
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
		UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem::TeleportToAnchor: якорь [%s] не найден"), *AnchorID.ToString());
		return false;
	}

	const FVector TargetLocation  = FoundAnchor->GetActorLocation();
	const FRotator TargetRotation = FoundAnchor->GetActorRotation();

	APlayerController* PC = W->GetFirstPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem::TeleportToAnchor: нет PlayerController"));
		return false;
	}

	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->TeleportTo(TargetLocation, TargetRotation);
		PC->SetControlRotation(TargetRotation);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem::TeleportToAnchor: телепорт в '%s' pos=%s rot=%s"),
			*FoundAnchor->DisplayName.ToString(), *TargetLocation.ToString(), *TargetRotation.ToString());
	}
	else
	{
		SetPendingSpawnTransform(TargetLocation, TargetRotation);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem::TeleportToAnchor: Pawn отсутствует, сохранён PendingSpawn"));
	}

	return true;
}

// -----------------------------------------------------------------------------
// Pending Spawn Transform API
// -----------------------------------------------------------------------------

void UInteriorSubsystem::SetPendingSpawnTransform(const FVector& Location, const FRotator& Rotation)
{
	bHasPendingSpawn     = true;
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
	bHasPendingSpawn     = false;
	PendingSpawnLocation = FVector::ZeroVector;
	PendingSpawnRotation = FRotator::ZeroRotator;
}

bool UInteriorSubsystem::HasPendingSpawnTransform() const
{
	return bHasPendingSpawn;
}

// -----------------------------------------------------------------------------
// Pending Anchor ID API
// -----------------------------------------------------------------------------

void UInteriorSubsystem::SetPendingAnchorID(const FGuid& AnchorID)  { PendingAnchorID = AnchorID; }
FGuid UInteriorSubsystem::GetPendingAnchorID() const                { return PendingAnchorID; }
void UInteriorSubsystem::ClearPendingAnchorID()                     { PendingAnchorID.Invalidate(); }
bool UInteriorSubsystem::HasPendingAnchorID() const                 { return PendingAnchorID.IsValid(); }

// -----------------------------------------------------------------------------
// Subscribe / Unsubscribe
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
		Unreg(PlacementRegisterHandle,   PlacementRegisterConditionAsset);
		Unreg(PlacementUnregisterHandle, PlacementUnregisterConditionAsset);
	}
}

// -----------------------------------------------------------------------------
// Initialize / Deinitialize
// -----------------------------------------------------------------------------

void UInteriorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Регистрируемся в GameSaveSubsystem для участия в сохранении/загрузке
	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->RegisterSaveableSubsystem(this);
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		CachedEventBus = GI->GetSubsystem<UEventBusSubsystem>();
	}

	// Подписываемся на загрузку карты (срабатывает после SeamlessTravel)
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UInteriorSubsystem::OnPostLoadMap);

	SubscribeAll();

	//SubscribeToSpawnActor();
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

	// Отписываемся от загрузки карты
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	PostLoadMapHandle.Reset();

	RegisteredItems.Empty();
	RegistrationListeners.Empty();
	SpawnedActorsByInteriorFloor.Empty();
	//FloorStateSnapshots.Empty();
	MissionFloorSnapshots.Empty();
	CachedEventBus = nullptr;

	UnsubscribeFromSpawnActor();

	Super::Deinitialize();
}

// Новая функция — обработчик завершения загрузки карты
void UInteriorSubsystem::OnPostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld) return;

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
		UE_LOG(LogTemp, Verbose, TEXT("InteriorSubsystem::OnPostLoadMap: нет PendingAnchorID"));
		return;
	}

	const FGuid AnchorID = GetPendingAnchorID();

	UE_LOG(LogTemp, Log,
		TEXT("InteriorSubsystem::OnPostLoadMap: the map is loaded, searching for the anchor [%s]"),
		*AnchorID.ToString());

	// Небольшая задержка — дождаться создания акторов на уровне
	FTimerHandle TimerHandle;
	FTimerDelegate Delegate = FTimerDelegate::CreateLambda([this, AnchorID, LoadedWorld, bNeedTeleportToAnchor]()
	{
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

		// Если нашли якорь — извлекаем связанный FloorAsset (если есть) и формируем CurrentKey
		
		bool bHaveKey = false;
		if (FoundAnchor)
		{
			UObject* OwnerAsset = FoundAnchor->GetOwnerRegistrationAsset();
			if (UFloorAsset* FloorAsset = Cast<UFloorAsset>(OwnerAsset))
			{
				if (FloorAsset->FloorID.IsValid())
				{
					FGuid InteriorSetId;
					InteriorSetId = FloorAsset->InteriorSetID;
					UInteriorSetAsset* LoadedSet = FloorAsset->ParentInteriorSet.LoadSynchronous();
					if (!InteriorSetId.IsValid() && FloorAsset->ParentInteriorSet.IsValid())
					{
						// Обратная совместимость
						if (LoadedSet)
						{
							InteriorSetId = LoadedSet->InteriorSetID;
						}
					}					
					CurrentKey = FInteriorFloorKey(InteriorSetId, FloorAsset->FloorID);
					bHaveKey = true;
				}
			}
		}

		// Внутри лямбды, после того как определили bHaveKey и CurrentKey
		if (bHaveKey)
		{
			//SubscribePlacementRegistration();
			SubscribeToSpawnActor();
			// 2. Восстанавливаем базовые снапшоты состояния (FloorStateSnapshots)
			if (const TArray<FFloorSavedActorState>* BaseSnap = FloorStateSnapshots.Find(CurrentKey))
			{
				if (BaseSnap && !BaseSnap->IsEmpty())
				{
					UWorld* LocalWorld = GetWorld();
					if (LocalWorld)
					{
						RestoreSpawnedActorsForCurrentFloor(FMissionEnvelope());

						RestoreFromSnapshotArray(LocalWorld, *BaseSnap, CurrentKey.InteriorSetId, CurrentKey.FloorId, FMissionEnvelope());
						UE_LOG(LogTemp, Log,
							TEXT("InteriorSubsystem::OnPostLoadMap: Restored baseline FloorStateSnapshots for floor %s/%s (count=%d)"),
							*CurrentKey.InteriorSetId.ToString(), *CurrentKey.FloorId.ToString(), BaseSnap->Num());
					}
				}
			}

			// 3. Восстанавливаем миссионные снапшоты (если активна миссия)
			if (const TMap<FName, TArray<FFloorSavedActorState>>* PerFloor = MissionFloorSnapshots.Find(CurrentKey))
			{
				if (PerFloor->Num() > 0)
				{
					for (const auto& MissionPair : *PerFloor)
					{
						const FName& MissionId = MissionPair.Key;
						if (!ActiveMissions.Contains(MissionId))
						{
							RestoreSpawnedActorsForCurrentFloor(FMissionEnvelope());

							if (const TArray<FFloorSavedActorState>* BaseSnap = FloorStateSnapshots.Find(CurrentKey))
							{
								if (BaseSnap && !BaseSnap->IsEmpty())
								{
									UWorld* LocalWorld = GetWorld();
									if (LocalWorld)
									{
										RestoreFromSnapshotArray(LocalWorld, *BaseSnap, CurrentKey.InteriorSetId, CurrentKey.FloorId, FMissionEnvelope());
										UE_LOG(LogTemp, Log,
											TEXT("InteriorSubsystem::OnPostLoadMap: Restored baseline FloorStateSnapshots for floor %s/%s (count=%d)"),
											*CurrentKey.InteriorSetId.ToString(), *CurrentKey.FloorId.ToString(), BaseSnap->Num());
									}
								}
							}

							continue;
						}

						//ApplyPersistentPopulation(CurrentKey);

						int32 CurrentMissionStep = ActiveMissions[MissionId].MissionStep;
						RestoreMissionFloorState(MissionId, CurrentMissionStep, CurrentKey);
						UE_LOG(LogTemp, Log,
							TEXT("InteriorSubsystem::OnPostLoadMap: Restored mission snapshot for mission '%s' floor %s/%s"),
							*MissionId.ToString(), *CurrentKey.InteriorSetId.ToString(), *CurrentKey.FloorId.ToString());
					}
				}
			}
		}
		// Финальная нотификация о завершении загрузки/перехода
		OnTransitionCompleted.Broadcast(bOk, TransitionPayloadCache ? TransitionPayloadCache->DestinationLink : FLocationAnchorLink(), true);
		ClearPendingAnchorID();
		//IsLoadingFromSave = false;
	});

	LoadedWorld->GetTimerManager().SetTimer(TimerHandle, Delegate, 0.5f, false);
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

	UnregisterHandle(SpawnRegisterHandle,   SpawnConditionAsset);
		UnregisterHandle(DespawnRegisterHandle, DespawnConditionAsset);

		UnregisterHandle(PlacementRegisterHandle,   PlacementRegisterConditionAsset);
		UnregisterHandle(PlacementUnregisterHandle, PlacementUnregisterConditionAsset);

		UnregisterHandle(InteractCommandHandle, InteractCommandConditionAsset);
		UnregisterHandle(SetEnabledHandle,      SetEnabledConditionAsset);
		UnregisterHandle(SetRangeHandle,        SetRangeConditionAsset);
		UnregisterHandle(SetTooltipHandle,      SetTooltipConditionAsset);

		UnregisterHandle(FloorStateSaveHandle,    FloorStateSaveConditionAsset);
		UnregisterHandle(FloorStateRestoreHandle, FloorStateRestoreConditionAsset);
		UnregisterHandle(FloorTransitionHandle,   FloorTransitionConditionAsset);
	}

	RegisteredItems.Empty();
	RegistrationListeners.Empty();
}

// ─────────────────────────────────────────────────────────────────────────────
// Mission Snapshot API
// ─────────────────────────────────────────────────────────────────────────────

void UInteriorSubsystem::SaveMissionFloorState(FName MissionId, const FInteriorFloorKey& FloorKey)
{
	if (MissionId.IsNone() || !FloorKey.FloorId.IsValid()) return;

	EJobSpacePolicy Policy = EJobSpacePolicy::Freeze;
	TArray<FEnvelopeChannelEntry> Channels;

	SaveFloorActorsState(FloorKey.InteriorSetId, FloorKey.FloorId, MissionId, EMissionEndReason::None);

	UE_LOG(LogTemp, Log,
		TEXT("InteriorSubsystem::SaveMissionFloorState: saved for mission '%s' floor %s/%s"),
		*MissionId.ToString(),
		*FloorKey.InteriorSetId.ToString(), *FloorKey.FloorId.ToString());
}

void UInteriorSubsystem::RestoreMissionFloorState(FName MissionId, int32 CurrentMissionStep, const FInteriorFloorKey& FloorKey)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (MissionId.IsNone() || !FloorKey.FloorId.IsValid()) return;

	// Найти per-floor map, затем entry по MissionId
	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: RestoreMissionFloorState MissionFloorSnapshots count before: %d"), MissionFloorSnapshots.Num());
	if (const TMap<FName, TArray<FFloorSavedActorState>>* PerFloor = MissionFloorSnapshots.Find(FloorKey))
	{
		if (const TArray<FFloorSavedActorState>* Snapshot = PerFloor->Find(MissionId))
		{
			if (!Snapshot || Snapshot->IsEmpty()) return;

			auto MissionInterior = ActiveMissions.Find(MissionId);
			auto& Controller = MissionInterior->Controller;
			auto MissionAsset = Controller->GetMissionAsset();
			auto& Envelope = MissionAsset->Envelopes[CurrentMissionStep];

			RestoreSpawnedActorsForCurrentFloor(Envelope);
			RestoreFromSnapshotArray(World, *Snapshot, FloorKey.InteriorSetId, FloorKey.FloorId, Envelope);
			UE_LOG(LogTemp, Log,
				TEXT("InteriorSubsystem::RestoreMissionFloorState: restored snapshot for mission '%s' floor %s/%s"),
				*MissionId.ToString(),
				*FloorKey.InteriorSetId.ToString(), *FloorKey.FloorId.ToString());
		}
	}
}

void UInteriorSubsystem::SpawnFromType(TArray<FFloorPopulationRecord>& Array, const FMissionEnvelope& Envelope)
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (const FFloorPopulationRecord& Record : Array)
	{
		if (!Record.SourceClass) continue;
		EEnvelopeChannel Channel = FloorActorTypeToEnvelopeChannel(Record.ActorType);

		if (!Record.IsNewObject)
		{
			RemoveRecordByItemId(SpawnedActorsByInteriorFloor, Record.ActorId);
			RemoveRecordByItemId(DestroyedActorsByInteriorFloor, Record.ActorId);
			continue;
		}
		else
		{
			if (!ShouldSkipActor(Envelope, Channel, EMissionEndReason::None, true))
			{
				RemoveRecordByItemId(SpawnedActorsByInteriorFloor, Record.ActorId);
				RemoveRecordByItemId(DestroyedActorsByInteriorFloor, Record.ActorId);
				continue;
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
				if(Record.ActorId == C->ItemId)
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
void UInteriorSubsystem::RestoreSpawnedActorsForCurrentFloor(const FMissionEnvelope& Envelope)
{
	TMap<FInteriorFloorKey, FFloorPopulationBuckets> Temp = SpawnedActorsByInteriorFloor;
	FFloorPopulationBuckets* Buckets = Temp.Find(CurrentKey);
	if (!Buckets) return;

	UWorld* World = GetWorld();

	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), TestAllActors);

	// Просто спавним акторы без восстановления состояния (состояние будет восстановлено позже из MissionFloorSnapshots)
	SpawnFromType(Buckets->HeavyFurniture, Envelope);
	SpawnFromType(Buckets->LightItems, Envelope);
	SpawnFromType(Buckets->Terminals, Envelope);
	SpawnFromType(Buckets->NPCSpawners, Envelope);
	SpawnFromType(Buckets->Debris, Envelope);

	TestAllActors.Empty();
}

const TMap<FInteriorFloorKey, TArray<FFloorSavedActorState>>* UInteriorSubsystem::GetMissionSnapshots(FName MissionId) const 
{
	// Сформировать временную карту этаж->snapshot для данной миссии и вернуть указатель на кеш
	MissionSnapshotQueryCache.Reset();
	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: GetMissionSnapshots MissionFloorSnapshots count before: %d"), MissionFloorSnapshots.Num());
	for (const auto& Pair : MissionFloorSnapshots)
	{
		const FInteriorFloorKey& FloorKey = Pair.Key;
		const TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = Pair.Value;
		if (const TArray<FFloorSavedActorState>* Snap = PerFloor.Find(MissionId))
		{
			MissionSnapshotQueryCache.FindOrAdd(FloorKey) = *Snap;
		}
	}
	return &MissionSnapshotQueryCache;
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
	for (const auto Pair : ActiveMissions)
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
	StoreSnapshot(FindedMissionId, FindedEnvelope, EMissionEndReason::None);


	OutData.SubsystemName = GetSaveSubsystemName();

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	// --- 1. Сериализация ActiveMissions (как было ранее, но с небольшими правками) ---
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

	// --- 2. Сериализация FloorStateSnapshots ---
	Root->SetField(TEXT("FloorStateSnapshots"), SerializeFloorStateSnapshots(FloorStateSnapshots));

	// --- 3. Сериализация MissionFloorSnapshots ---
	Root->SetField(TEXT("MissionFloorSnapshots"), SerializeMissionFloorSnapshots(MissionFloorSnapshots));

	// --- Сериализация SpawnedActorsByInteriorFloor и DestroyedActorsByInteriorFloor ---
	Root->SetField(TEXT("SpawnedActors"), SerializePopulationMap(SpawnedActorsByInteriorFloor));
	Root->SetField(TEXT("DestroyedActors"), SerializePopulationMap(DestroyedActorsByInteriorFloor));

	// Запись в строку
	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	OutData.SerializedData = Output;
}

void UInteriorSubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
	//IsLoadingFromSave = true;

	if (InData.SerializedData.IsEmpty()) return;

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem::ApplySaveData: failed to parse JSON"));
		return;
	}

	// --- 1. Восстановление списка активных миссий (сохраняем в контейнер для последующего использования) ---
	// Здесь мы не восстанавливаем сами миссии, а только сохраняем информацию для MissionSubsystem.
	// Само восстановление миссий выполнит MissionSubsystem при загрузке.
	// Однако мы можем заполнить временный массив, который позже будет передан в MissionSubsystem.
	// Пока просто прочитаем и залогируем.

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
			const EMissionStatus   Status = static_cast<EMissionStatus>(StatusInt);
			const EMissionEndReason EndReason = static_cast<EMissionEndReason>(EndReasonInt);
			const EMissionResumeMode Resume = static_cast<EMissionResumeMode>(ResumeModeInt);

			// Загрузить ассет
			UMissionAsset* Asset = Cast<UMissionAsset>(
				StaticLoadObject(UMissionAsset::StaticClass(), nullptr, *AssetPath));
			if (!Asset)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("MissionSubsystem::ApplySaveData: Cannot load MissionAsset '%s'"), *AssetPath);
				continue;
			}

			switch (Resume)
			{
			case EMissionResumeMode::RestartOnLoad:
			{
				UMissionController* Ctrl = CreateMission(Asset);
				if (Ctrl)
				{
					//ActivateMission(MissionId);
					Ctrl->MissionStep = 0;
					ActiveMissions.Find(MissionId)->MissionStep = Ctrl->MissionStep;
				}
				continue;
			}
			case EMissionResumeMode::Resumable:
			{
				UMissionController* Ctrl = CreateMission(Asset);
				if (Ctrl)
				{
					//ActivateMission(MissionId);
					Ctrl->MissionStep = MissionStep;
					ActiveMissions.Find(MissionId)->MissionStep = MissionStep;
				}
				continue;
			}
			case EMissionResumeMode::FailOnLoad:
			{
				UMissionController* Ctrl = CreateMission(Asset);
				if (!Ctrl) continue;

				// Восстанавливаем статус из сохранения
				if (Status == EMissionStatus::Active || Status == EMissionStatus::Suspended)
				{
				}
				else if (Status == EMissionStatus::Resolved)
				{
					Ctrl->MissionStep = Asset->Envelopes.Num() - 1;
					ActiveMissions.Find(MissionId)->MissionStep = Ctrl->MissionStep;
				}
				continue;
			}
			}

			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem::ApplySaveData: read mission '%s' (Status=%d)"),
				*MissionIdStr, StatusInt);
		}
	}

	// --- 2. Восстановление FloorStateSnapshots ---
	TSharedPtr<FJsonValue> FloorStateValue = Root->TryGetField(TEXT("FloorStateSnapshots"));
	if (FloorStateValue.IsValid())
	{
		DeserializeFloorStateSnapshots(FloorStateValue, FloorStateSnapshots);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem::ApplySaveData: restored FloorStateSnapshots, count=%d"), FloorStateSnapshots.Num());
	}

	// --- 3. Восстановление MissionFloorSnapshots ---
	TSharedPtr<FJsonValue> MissionFloorValue = Root->TryGetField(TEXT("MissionFloorSnapshots"));
	if (MissionFloorValue.IsValid())
	{
		MissionFloorSnapshots.Empty();
		DeserializeMissionFloorSnapshots(MissionFloorValue, MissionFloorSnapshots);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem::ApplySaveData: restored MissionFloorSnapshots, count=%d"), MissionFloorSnapshots.Num());
	}

	// --- Десериализация SpawnedActorsByInteriorFloor и DestroyedActorsByInteriorFloor ---
	TSharedPtr<FJsonValue> SpawnedValue = Root->TryGetField(TEXT("SpawnedActors"));
	if (SpawnedValue.IsValid())
	{
		DeserializePopulationMap(SpawnedValue, SpawnedActorsByInteriorFloor);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem::ApplySaveData: restored SpawnedActors, count=%d"), SpawnedActorsByInteriorFloor.Num());
	}
	TSharedPtr<FJsonValue> DestroyedValue = Root->TryGetField(TEXT("DestroyedActors"));

	if (DestroyedValue.IsValid())
	{
		DeserializePopulationMap(DestroyedValue, DestroyedActorsByInteriorFloor);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem::ApplySaveData: restored DestroyedActors, count=%d"), DestroyedActorsByInteriorFloor.Num());
	}
}

UMissionController* UInteriorSubsystem::CreateMission(UMissionAsset* MissionAsset)
{
	if (!MissionAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem::CreateMission: MissionAsset is null"));
		return nullptr;
	}

	const FName MissionId = MissionAsset->GetMissionId();

	if (ActiveMissions.Contains(MissionId))
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem::CreateMission: Mission '%s' already exists"),
			*MissionId.ToString());
		return ActiveMissions[MissionId].Controller;
	}
	// Создаём контроллер нужного класса (из ассета). Если в ассете задан Blueprint класс,
	// он будет инстанцирован и вызовы Activate()/OnMissionActivated будут попадать в BP-реализацию.
	UMissionController* Controller = nullptr;
	if (MissionAsset->ControllerClass && MissionAsset->ControllerClass->IsChildOf(UMissionController::StaticClass()))
	{
		Controller = NewObject<UMissionController>(GetGameInstance(), MissionAsset->ControllerClass);
	}
	else
	{
		// Фоллбек на базовый C++ контроллер
		Controller = NewObject<UMissionController>(GetGameInstance());
		if (!MissionAsset->ControllerClass)
		{
			UE_LOG(LogTemp, Verbose, TEXT("MissionSubsystem::CreateMission: ControllerClass not set in MissionAsset '%s', using default UMissionController"), *MissionId.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem::CreateMission: ControllerClass for MissionAsset '%s' is not a UMissionController subclass, using default"), *MissionId.ToString());
		}
	}

	Controller->InitFromAsset(MissionAsset, GetGameInstance());

	FActiveMissionInterior Entry;
	Entry.MissionId = MissionId;
	Entry.Controller = Controller;
	ActiveMissions.Add(MissionId, Entry);

	UE_LOG(LogTemp, Log, TEXT("MissionSubsystem::CreateMission: Created mission '%s'"), *MissionId.ToString());
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
		UE_LOG(LogTemp, Warning, TEXT("[UPDATE_MISSION] Received UpdateMissionListPayload. CurrentMissionId=%s, IsUpdateSnapshots=%d"),
			*P->CurrentMissionId.ToString(), P->IsUpdateSnapshots);

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
			UE_LOG(LogTemp, Warning, TEXT("[UPDATE_MISSION] Calling StoreCurrentLevel..."));
			StoreCurrentLevel(Envelope, P->CurrentMissionId, EMissionEndReason::Completed);
			UE_LOG(LogTemp, Warning, TEXT("[UPDATE_MISSION] Calling StoreSnapshot with Policy=%d"), (int32)Envelope.NextStagePolicy);
			StoreSnapshot(P->CurrentMissionId, Envelope, EMissionEndReason::Completed);
		}

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
				UE_LOG(LogTemp, Log, TEXT("[UPDATE_MISSION] Added active mission %s, step=%d"), *MissionId.ToString(), MissionInterior.MissionStep);
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


		// При завершении миссии — применить политику к сохранённым snapshot-ам для этой миссии
		auto MissionInterior = ActiveMissions.Find(MissionId);
		TArray<FEnvelopeChannelEntry> EndChannels;
		if (MissionInterior)
		{
			TObjectPtr<UMissionController> Controller = MissionInterior->Controller;
			UMissionAsset* MissionAsset = Controller->GetMissionAsset();
			FMissionEnvelope Envelope = MissionAsset->Envelopes[MissionInterior->MissionStep];

			EJobSpacePolicy Policy = EJobSpacePolicy::None;
			
			switch (P->EndReason)
			{
				case EMissionEndReason::Completed:
				{
					Policy =Envelope.NextStagePolicy;
					EndChannels = Envelope.NextStagePolicyChannels;
					break;
				}
				case EMissionEndReason::Failed:
				{
					Policy = Envelope.MissionFailedPolicy;
					EndChannels = Envelope.MissionFailedPolicyChannels;
					break;
				}
				case EMissionEndReason::Abandoned:
				{
					Policy = Envelope.MissionAbandonedPolicy;
					EndChannels = Envelope.MissionAbandonedChannels;
					break;
				}
			}

			StoreCurrentLevel(Envelope, MissionId, P->EndReason);
			StoreSnapshot(MissionId, Envelope, P->EndReason);

			ActiveMissions.Remove(MissionId);
			MissionFloorSnapshots.Find(CurrentKey)->Remove(MissionId);
		}
	}
}

void UInteriorSubsystem::StoreCurrentLevel(FMissionEnvelope Envelope, FName MissionId, EMissionEndReason EndReason)
{
	FMissionEnvelopeScope Scope = Envelope.Scope;
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[STORE_CURRENT] No world!"));
		return;
	}

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	UE_LOG(LogTemp, Warning, TEXT("[STORE_CURRENT] CurrentLevelName (raw) = '%s'"), *CurrentLevelName);

	//int32 ScopesFound = 0;
	for (const auto& S : Scope.InteriorScopes)
	{
		if (!S)
		{
			UE_LOG(LogTemp, Warning, TEXT("[STORE_CURRENT] Skipping null scope entry"));
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
				UE_LOG(LogTemp, Warning, TEXT("[STORE_CURRENT] InteriorSetID invalid and cannot load for floor %s"), *FloorGuid.ToString());
				continue;
			}
		}

		FString ScopeLevelName = S->FloorLevel.ToSoftObjectPath().GetLongPackageName();
		UE_LOG(LogTemp, Warning, TEXT("[STORE_CURRENT] ScopeLevelName (raw) = '%s'"), *ScopeLevelName);

		FString NormTarget = NormalizeLevelName(ScopeLevelName);
		UE_LOG(LogTemp, Warning, TEXT("[STORE_CURRENT] NormTarget = '%s'"), *NormTarget);

		// Также нормализуем текущее имя уровня (на случай если оно с префиксом)
		FString NormCurrent = NormalizeLevelName(CurrentLevelName);
		UE_LOG(LogTemp, Warning, TEXT("[STORE_CURRENT] NormCurrent = '%s'"), *NormCurrent);

		if (NormTarget == NormCurrent)
		{
			UE_LOG(LogTemp, Warning, TEXT("[STORE_CURRENT] MATCH! Saving state for floor %s/%s"), *InteriorSetID.ToString(), *FloorGuid.ToString());
			SaveFloorActorsState(InteriorSetID, FloorGuid, MissionId, EndReason);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[STORE_CURRENT] NO MATCH: NormTarget='%s' != NormCurrent='%s'"), *NormTarget, *NormCurrent);
		}
	}
}

void UInteriorSubsystem::StoreSnapshot(FName MissionId, FMissionEnvelope Envelope, EMissionEndReason EndReason)
{
	EJobSpacePolicy Policy = EJobSpacePolicy::None;
	TArray<FEnvelopeChannelEntry> EndChannels;

	switch (EndReason)
	{
	case EMissionEndReason::None:
	{
		Policy = Envelope.RuntimePolicy;
		break;
	}
	case EMissionEndReason::Completed:
	{
		Policy = Envelope.NextStagePolicy;
		break;
	}
	case EMissionEndReason::Failed:
	{
		Policy = Envelope.MissionFailedPolicy;
		break;
	}
	case EMissionEndReason::Abandoned:
	{
		Policy = Envelope.MissionAbandonedPolicy;
		break;
	}
	}

	switch (Policy)
	{
	case EJobSpacePolicy::Reset:
	{
		UE_LOG(LogTemp, Warning, TEXT("[STORE] Reset policy: clearing persistent maps for all floors in MissionFloorSnapshots"));
		if(auto Map = MissionFloorSnapshots.Find(CurrentKey))
		{
			Map->Remove(MissionId);
		}

		if (auto Array = FloorStateSnapshots.Find(CurrentKey))
		{
			Array->Empty();
		}
		
		if (FFloorPopulationBuckets* PersSpawned = SpawnedActorsByInteriorFloor.Find(CurrentKey))
		{
			RemoveRecordsForChannel(PersSpawned->HeavyFurniture, FloorActorTypeToEnvelopeChannel(EFloorActorType::HeavyFurniture));
			RemoveRecordsForChannel(PersSpawned->LightItems, FloorActorTypeToEnvelopeChannel(EFloorActorType::LightItem));
			RemoveRecordsForChannel(PersSpawned->Terminals, FloorActorTypeToEnvelopeChannel(EFloorActorType::Terminal));
			RemoveRecordsForChannel(PersSpawned->NPCSpawners, FloorActorTypeToEnvelopeChannel(EFloorActorType::NPC_Spawner));
			RemoveRecordsForChannel(PersSpawned->Debris, FloorActorTypeToEnvelopeChannel(EFloorActorType::Debris));
		}
		if (FFloorPopulationBuckets* PersDestroyed = DestroyedActorsByInteriorFloor.Find(CurrentKey))
		{
			RemoveRecordsForChannel(PersDestroyed->HeavyFurniture, FloorActorTypeToEnvelopeChannel(EFloorActorType::HeavyFurniture));
			RemoveRecordsForChannel(PersDestroyed->LightItems, FloorActorTypeToEnvelopeChannel(EFloorActorType::LightItem));
			RemoveRecordsForChannel(PersDestroyed->Terminals, FloorActorTypeToEnvelopeChannel(EFloorActorType::Terminal));
			RemoveRecordsForChannel(PersDestroyed->NPCSpawners, FloorActorTypeToEnvelopeChannel(EFloorActorType::NPC_Spawner));
			RemoveRecordsForChannel(PersDestroyed->Debris, FloorActorTypeToEnvelopeChannel(EFloorActorType::Debris));
		}

		break;
	}
	case EJobSpacePolicy::Freeze:
	{
		UE_LOG(LogTemp, Warning, TEXT("[STORE] Freeze policy: copying mission snapshots and dynamic maps to persistent storage"));

		// Статические снапшоты свойств
		for (auto& Pair : MissionFloorSnapshots)
		{
			const FInteriorFloorKey& FloorKey = Pair.Key;
			TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = Pair.Value;
			if (!PerFloor.Contains(MissionId)) continue;
			FloorStateSnapshots.Add(FloorKey, PerFloor[MissionId]);
			UE_LOG(LogTemp, Warning, TEXT("[STORE]   Copied actor state snapshots for floor %s/%s, count=%d"),
				*FloorKey.InteriorSetId.ToString(), *FloorKey.FloorId.ToString(), PerFloor[MissionId].Num());
		}
		break;
	}

	

	case EJobSpacePolicy::Partial:
	{
		switch (EndReason)
		{
		case EMissionEndReason::None:
		{
			EndChannels = Envelope.RuntimePolicyChannels;
			break;
		}
		case EMissionEndReason::Completed:
		{
			EndChannels = Envelope.NextStagePolicyChannels;
			break;
		}
		case EMissionEndReason::Failed:
		{
			EndChannels = Envelope.MissionFailedPolicyChannels;
			break;
		}
		case EMissionEndReason::Abandoned:
		{
			EndChannels = Envelope.MissionAbandonedChannels;
			break;
		}
	}
			
		for (const FEnvelopeChannelEntry& ChannelEntry : EndChannels)
		{
			UE_LOG(LogTemp, Warning, TEXT("[STORE]   Channel %d, Policy %d"), (int32)ChannelEntry.Channel, (int32)ChannelEntry.Policy);

			if (ChannelEntry.Policy == EChannelPolicy::Freeze)
			{
				// 1) Копируем статические снапшоты для этого канала
				for (auto& Pair : MissionFloorSnapshots)
				{
					const FInteriorFloorKey& FloorKey = Pair.Key;
					TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = Pair.Value;
					if (!PerFloor.Contains(MissionId)) continue;
					const TArray<FFloorSavedActorState>& MissionSnap = PerFloor[MissionId];
					TArray<FFloorSavedActorState>& BaseSnap = FloorStateSnapshots.FindOrAdd(FloorKey);

					for (const FFloorSavedActorState& ActorSnap : MissionSnap)
					{
						FGuid FoundItemId;
						EEnvelopeChannel ActorChannel = EEnvelopeChannel::None;
						if (UWorld* W = GetWorld())
						{
							for (TActorIterator<AActor> It(W); It; ++It)
							{
								AActor* Actor = *It;
								if (!IsValid(Actor)) continue;
								if (UFloorAssignmentComponent* FAC = Actor->FindComponentByClass<UFloorAssignmentComponent>())
								{
									if (FAC->SnapshotChannel == ESnapshotChannel::None) continue;

									if (FAC->ItemId == ActorSnap.ItemId)
									{
										FoundItemId = FAC->ItemId;
										ActorChannel = FloorActorTypeToEnvelopeChannel(FAC->ActorType);
										break;
									}
								}
							}
						}

						if (ActorChannel == EEnvelopeChannel::None) continue;

						if (ActorChannel == ChannelEntry.Channel)
						{
							BaseSnap.RemoveAll([&FoundItemId](const FFloorSavedActorState& State)
								{
									return State.ItemId == FoundItemId;
								});

							BaseSnap.Add(ActorSnap);
						}
					}
				}
				
				// 2) Копируем динамические карты для этого канала
				for (auto& Pair : MissionFloorSnapshots)
				{
					const FInteriorFloorKey& FloorKey = Pair.Key;

					if (FloorKey != CurrentKey)
						continue;

					FFloorPopulationBuckets& PersSpawned = SpawnedActorsByInteriorFloor.FindOrAdd(FloorKey);
					FFloorPopulationBuckets& PersDestroyed = DestroyedActorsByInteriorFloor.FindOrAdd(FloorKey);

					if (auto Temp = FloorStateSnapshots.Find(FloorKey))
					{
						if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::HeavyFurniture), EndReason, true))
						{
							PersSpawned.HeavyFurniture.Empty();
						}

						if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::LightItem), EndReason, true))
						{
							PersSpawned.LightItems.Empty();
						}
						
						if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::Terminal), EndReason, true))
						{
							PersSpawned.Terminals.Empty();
						}

						if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::NPC_Spawner), EndReason, true))
						{
							PersSpawned.NPCSpawners.Empty();
						}

						if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::Debris), EndReason, true))
						{
							PersSpawned.Debris.Empty();
						}
					}
					if (const FFloorPopulationBuckets* TempDestroyed = DestroyedActorsByInteriorFloor.Find(FloorKey))
					{
						if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::HeavyFurniture), EndReason, true))
						{
							PersDestroyed.HeavyFurniture.Empty();
						}
						
						if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::LightItem), EndReason, true))
						{
							PersDestroyed.LightItems.Empty();
						}

						if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::Terminal), EndReason, true))
						{
							PersDestroyed.Terminals.Empty();
						}

						if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::NPC_Spawner), EndReason, true))
						{
							PersDestroyed.NPCSpawners.Empty();
						}
						
						if (!ShouldSkipActor(Envelope, FloorActorTypeToEnvelopeChannel(EFloorActorType::Debris), EndReason, true))
						{
							PersDestroyed.Debris.Empty();
						}
					}
				}
			}
			else if (ChannelEntry.Policy == EChannelPolicy::Reset)
			{
				for (auto& Pair : MissionFloorSnapshots)
				{
					const FInteriorFloorKey& FloorKey = Pair.Key;
					if (FFloorPopulationBuckets* PersSpawned = SpawnedActorsByInteriorFloor.Find(FloorKey))
					{
						RemoveRecordsForChannel(PersSpawned->HeavyFurniture, ChannelEntry.Channel);
						RemoveRecordsForChannel(PersSpawned->LightItems, ChannelEntry.Channel);
						RemoveRecordsForChannel(PersSpawned->Terminals, ChannelEntry.Channel);
						RemoveRecordsForChannel(PersSpawned->NPCSpawners, ChannelEntry.Channel);
						RemoveRecordsForChannel(PersSpawned->Debris, ChannelEntry.Channel);
					}
					if (FFloorPopulationBuckets* PersDestroyed = DestroyedActorsByInteriorFloor.Find(FloorKey))
					{
						RemoveRecordsForChannel(PersDestroyed->HeavyFurniture, ChannelEntry.Channel);
						RemoveRecordsForChannel(PersDestroyed->LightItems, ChannelEntry.Channel);
						RemoveRecordsForChannel(PersDestroyed->Terminals, ChannelEntry.Channel);
						RemoveRecordsForChannel(PersDestroyed->NPCSpawners, ChannelEntry.Channel);
						RemoveRecordsForChannel(PersDestroyed->Debris, ChannelEntry.Channel);
					}
				}
			}
		}
		break;
	}
	case EJobSpacePolicy::None:
	{
		UE_LOG(LogTemp, Log, TEXT("[STORE] None policy, nothing stored"));
		return;
	}
	}
}

void UInteriorSubsystem::ReleaseMissionSnapshot(FName MissionId, const FMissionEnvelope& Envelope, EJobSpacePolicy Policy, bool bIsCompletion /*= false*/)
{
	if (MissionId.IsNone()) return;

	UWorld* World = GetWorld();

	// Если это просто покидание этажа во время миссии — ничего дополнительно не делать:
	if (!bIsCompletion)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("InteriorSubsystem::ReleaseMissionSnapshot: floor-exit for mission '%s', policy=%d — mission snapshot preserved"),
			*MissionId.ToString(), static_cast<int32>(Policy));
		return;
	}

	// Обработка завершения миссии (bIsCompletion == true)
	switch (Policy)
	{
	case EJobSpacePolicy::Reset:
	{
		// ResetAll: восстановить из базового FloorStateSnapshots (если есть)
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: ReleaseMissionSnapshot MissionFloorSnapshots count before: %d"), MissionFloorSnapshots.Num());
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
					UE_LOG(LogTemp, Log,
						TEXT("InteriorSubsystem::ReleaseMissionSnapshot: ResetAll — restored FloorStateSnapshots for floor %s (mission '%s')"),
						*FloorKey.FloorId.ToString(), *MissionId.ToString());
				}
			}
			else
			{
				UE_LOG(LogTemp, Verbose,
					TEXT("InteriorSubsystem::ReleaseMissionSnapshot: ResetAll — no FloorStateSnapshots baseline for floor %s"),
					*FloorKey.FloorId.ToString());
			}
		}
		break;
	}

	case EJobSpacePolicy::Freeze:
	{
		// FreezeAll: копируем mission snapshot в постоянное хранилище FloorStateSnapshots
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: ReleaseMissionSnapshot Policy freeze MissionFloorSnapshots count before: %d"), MissionFloorSnapshots.Num());
		for (auto& Pair : MissionFloorSnapshots)
		{
			const FInteriorFloorKey& FloorKey = Pair.Key;
			TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = Pair.Value;
			if (const TArray<FFloorSavedActorState>* MissionSnap = PerFloor.Find(MissionId))
			{
				FloorStateSnapshots.FindOrAdd(FloorKey) = *MissionSnap;
				UE_LOG(LogTemp, Log,
					TEXT("InteriorSubsystem::ReleaseMissionSnapshot: FreezeAll — wrote MissionFloorSnapshot to FloorStateSnapshots for floor %s (mission '%s')"),
					*FloorKey.FloorId.ToString(), *MissionId.ToString());
			}
		}
		break;
	}

	case EJobSpacePolicy::Partial:
	{
		// Partial: применяем политику поканально
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: ReleaseMissionSnapshot Policy Partial MissionFloorSnapshots count before: %d"), MissionFloorSnapshots.Num());
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
							UE_LOG(LogTemp, Log,
								TEXT("InteriorSubsystem::ReleaseMissionSnapshot: Partial-Reset — restored FloorStateSnapshots for floor %s (mission '%s')"),
								*FloorKey.FloorId.ToString(), *MissionId.ToString());
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

	// Очищаем mission snapshots для этой миссии
	for (auto It = MissionFloorSnapshots.CreateIterator(); It; ++It)
	{
		TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = It->Value;
		PerFloor.Remove(MissionId);
		if (PerFloor.Num() == 0)
			It.RemoveCurrent();
	}

	UE_LOG(LogTemp, Log,
		TEXT("InteriorSubsystem::ReleaseMissionSnapshot: completed for mission '%s' (policy=%d)"),
		*MissionId.ToString(), static_cast<int32>(Policy));
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

	// Если актор с таким ItemId уже зарегистрирован в SpawnedActorsByInteriorFloor, пропускаем регистрацию
	if (ContainsActorIdInSpawned(SpawnedActorsByInteriorFloor, Comp->ItemId))
	{
		UE_LOG(LogTemp, Verbose, TEXT("TryRegisterActor: Actor %s already registered (ItemId %s), skipping registration"),
			*SpawnedActor->GetName(), *Comp->ItemId.ToString());
		return;
	}

	if (Comp->ItemId.IsValid())
	{
		// публикуем событие
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
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TryRegisterActor: ItemId not set after attempts for actor %s"), *SpawnedActor->GetName());
	}
}

void UInteriorSubsystem::CopyRecordsForChannel(const TArray<FFloorPopulationRecord>& Source, TArray<FFloorPopulationRecord>& Dest, EEnvelopeChannel Channel)
{
	CopyRecordsForChannelHelper(Source, Dest, Channel);
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
		if (FindActorByItemId(Rec.ActorId)) continue; // не спавним повторно
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

void UInteriorSubsystem::ApplyPersistentPopulation(const FInteriorFloorKey& FloorKey)
{
	UE_LOG(LogTemp, Warning, TEXT("[APPLY] ApplyPersistentPopulation for floor %s/%s"),
		*FloorKey.InteriorSetId.ToString(), *FloorKey.FloorId.ToString());

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[APPLY] No world!"));
		return;
	}

	// Спавним объекты из PersistentSpawnedActors
	if (const FFloorPopulationBuckets* PersSpawned = SpawnedActorsByInteriorFloor.Find(FloorKey))
	{
		UE_LOG(LogTemp, Warning, TEXT("[APPLY]   PersistentSpawnedActors found: Heavy=%d, Light=%d, Terminals=%d, NPCSpawners=%d, Debris=%d"),
			PersSpawned->HeavyFurniture.Num(), PersSpawned->LightItems.Num(), PersSpawned->Terminals.Num(),
			PersSpawned->NPCSpawners.Num(), PersSpawned->Debris.Num());
		SpawnRecords(PersSpawned->HeavyFurniture);
		SpawnRecords(PersSpawned->LightItems);
		SpawnRecords(PersSpawned->Terminals);
		SpawnRecords(PersSpawned->NPCSpawners);
		SpawnRecords(PersSpawned->Debris);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[APPLY]   No PersistentSpawnedActors for this floor"));
	}

	// Уничтожаем объекты из PersistentDestroyedActors
	if (const FFloorPopulationBuckets* PersDestroyed = DestroyedActorsByInteriorFloor.Find(FloorKey))
	{
		int32 TotalDestroyed = PersDestroyed->HeavyFurniture.Num() + PersDestroyed->LightItems.Num() +
			PersDestroyed->Terminals.Num() + PersDestroyed->NPCSpawners.Num() + PersDestroyed->Debris.Num();
		UE_LOG(LogTemp, Warning, TEXT("[APPLY]   PersistentDestroyedActors found: total %d objects to destroy"), TotalDestroyed);
		DestroyRecords(PersDestroyed->HeavyFurniture);
		DestroyRecords(PersDestroyed->LightItems);
		DestroyRecords(PersDestroyed->Terminals);
		DestroyRecords(PersDestroyed->NPCSpawners);
		DestroyRecords(PersDestroyed->Debris);
	}

	// Очищаем временные карты для этого этажа (чтобы повторно не спавнить)
	SpawnedActorsByInteriorFloor.Remove(FloorKey);
	DestroyedActorsByInteriorFloor.Remove(FloorKey);
	UE_LOG(LogTemp, Verbose, TEXT("[APPLY] Cleared temporary maps for floor %s/%s"), *FloorKey.InteriorSetId.ToString(), *FloorKey.FloorId.ToString());
}
