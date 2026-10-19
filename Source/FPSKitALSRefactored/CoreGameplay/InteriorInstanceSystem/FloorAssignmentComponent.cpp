#include "FloorAssignmentComponent.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "FloorPlacementPayload.h"
#include "InteriorSubsystem.h"
#include "SaveSubsystems/WS_ActorRegistry.h"
#include "FloorAsset.h"
#include "../LocationSystem/InteriorSetAsset.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#else
#include "../LocationSystem/LocationAnchorActor.h"
#endif

namespace
{
	static FString NormalizeLevelName_FA(const FString& InPath)
	{
		if (InPath.IsEmpty()) return FString();

		FString PackagePath = InPath;
		if (PackagePath.Contains(TEXT(".")))
			PackagePath = FPackageName::ObjectPathToPackageName(PackagePath);

		FString Base = FPaths::GetBaseFilename(PackagePath);

		const int32 PIEPos = Base.Find(TEXT("UEDPIE_"), ESearchCase::IgnoreCase);
		if (PIEPos != INDEX_NONE)
		{
			const int32 MPos = Base.Find(TEXT("_M_"), ESearchCase::IgnoreCase, ESearchDir::FromStart, PIEPos);
			if (MPos != INDEX_NONE && MPos + 3 < Base.Len())
				Base = Base.Mid(MPos + 3);
			else if (PIEPos + 6 < Base.Len())
				Base = Base.Mid(PIEPos + 6);
		}

		return Base.ToLower();
	}

	/** Общий код применения найденного FloorAsset к компоненту.
	 *  Всегда перезаписывает FloorId/InteriorSetId/… , ItemId не перегенерируется
	 *  если уже валиден. */
	static bool ApplyFloorToComponent_FA(
		UFloorAssignmentComponent* Comp,
		UFloorAsset* Floor)
	{
		if (!Comp || !IsValid(Floor) || !Floor->FloorID.IsValid())
			return false;

		Comp->FloorId = Floor->FloorID;
		Comp->FloorName = Floor->DisplayName.IsEmpty()
			? FText::FromName(Floor->GetFName())
			: Floor->DisplayName;

		Comp->InteriorSetId.Invalidate();
		Comp->InteriorSetName = FText::GetEmpty();

		if (Floor->InteriorSetID.IsValid())
			Comp->InteriorSetId = Floor->InteriorSetID;

		if (!Floor->ParentInteriorSet.IsNull())
		{
			if (UInteriorSetAsset* IS = Floor->ParentInteriorSet.LoadSynchronous())
			{
				if (!Comp->InteriorSetId.IsValid())
					Comp->InteriorSetId = IS->InteriorSetID;

				Comp->InteriorSetName = IS->DisplayName.IsEmpty()
					? FText::FromName(IS->GetFName())
					: IS->DisplayName;
			}
		}

		return true;
	}
}

UFloorAssignmentComponent::UFloorAssignmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFloorAssignmentComponent::BeginPlay()
{
	Super::BeginPlay();

	AutoFillFloorIdsFromWorld();

	if (ProtectedItemId.IsValid())
	{
		ItemId = ProtectedItemId;
	}

	// Register the actor in the Actor Registry world subsystem
	if (GetWorld())
	{
		if (UWS_ActorRegistry* RegistrySubSystem = GetWorld()->GetSubsystem<UWS_ActorRegistry>())
		{
			RegistrySubSystem->RegisterActor(GetOwner(), ItemId);
		}
	}

	if (SnapshotChannel == ESnapshotChannel::Snapshot && ItemId.IsValid())
	{
		if (UWorld* W = GetWorld())
		{
			if (UGameInstance* GI = W->GetGameInstance())
			{
				if (UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>())
				{
					UFloorPlacementPayload* Payload = Bus->CreatePayload<UFloorPlacementPayload>();
					if (Payload)
					{
						AActor* Owner = GetOwner();
						TArray<FName> TextTags;
						if (Owner)
						{
							TextTags = Owner->Tags;
						}
						Payload->SetupWithTags(
							ActorType,
							Owner ? Owner->GetActorTransform() : FTransform::Identity,
							ItemId,
							Owner ? Owner->GetClass() : nullptr,
							this->GameplayTagContainer,
							TextTags
							, false);

						IsRuntimeSpawned = false;
						FOutcomeEventBase Ev;
						Ev.OutcomeType = EOutcomeType::Interior;
						Ev.OutcomeInterior = EOutcomeInterior::FloorPlacementRegistered;
						Ev.Payload = Payload;
						Bus->PublishOutcome(Ev);
					}
				}
			}
		}
	}
	if (ItemId.IsValid())
	{
		if (UInteriorSubsystem* Subsystem = GetInteriorSubsystem())
		{
			Subsystem->RegisterFloorActor(GetOwner(), ItemId);
		}
	}
}

void UFloorAssignmentComponent::OnRegister()
{
	Super::OnRegister();

	// В редакторе BeginPlay не вызывается, поэтому авто-заполнение делаем здесь —
	// это сработает при размещении актора на сцене, загрузке уровня и undo/redo.
	// В рантайме вызов безопасен: если данные уже заполнены из BeginPlay,
	// повторное заполнение даст тот же результат.
	AutoFillFloorIdsFromWorld();
}

void UFloorAssignmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ItemId.IsValid())
	{
		if (UInteriorSubsystem* Subsystem = GetInteriorSubsystem())
		{
			Subsystem->UnregisterFloorActor(ItemId);
		}
	}

	// UnRegister the actor in the Actor Registry world subsystem
	if (GetWorld())
	{
		if (UWS_ActorRegistry* RegistrySubSystem = GetWorld()->GetSubsystem<UWS_ActorRegistry>())
		{
			RegistrySubSystem->UnregisterActor(ItemId);
		}
	}

	if (SnapshotChannel == ESnapshotChannel::None || !ItemId.IsValid())
		return;

	if (EndPlayReason != EEndPlayReason::Destroyed)
		return;

	if (UWorld* W = GetWorld())
	{
		if (UGameInstance* GI = W->GetGameInstance())
		{
			if (UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>())
			{
				UFloorPlacementPayload* Payload = Bus->CreatePayload<UFloorPlacementPayload>();
				if (Payload)
				{
					AActor* Owner = GetOwner();
					TArray<FName> TextTags;
					if (Owner)
					{
						TextTags = Owner->Tags;
					}
					Payload->SetupWithTags(
						ActorType,
						Owner ? Owner->GetActorTransform() : FTransform::Identity,
						ItemId,
						Owner ? Owner->GetClass() : nullptr,
						this->GameplayTagContainer,
						TextTags,
						IsRuntimeSpawned
					);
					FOutcomeEventBase Ev;
					Ev.OutcomeType = EOutcomeType::Interior;
					Ev.OutcomeInterior = EOutcomeInterior::FloorPlacementUnregistered;
					Ev.Payload = Payload;
					Bus->PublishOutcome(Ev);
				}
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UFloorAssignmentComponent::PostEditImport()
{
	Super::PostEditImport();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// Свежий идентификатор — при копировании актора ItemId должен стать новым.
		ItemId = FGuid::NewGuid();
		ProtectedItemId = ItemId;

		// В редакторе данные этажа перечитываются из ассетов под текущий уровень.
		AutoFillFloorIdsFromWorld();

		MarkPackageDirty();
	}
}

UInteriorSubsystem* UFloorAssignmentComponent::GetInteriorSubsystem() const
{
	if (UWorld* W = GetWorld())
	{
		if (UGameInstance* GI = W->GetGameInstance())
		{
			return GI->GetSubsystem<UInteriorSubsystem>();
		}
	}
	return nullptr;
}

void UFloorAssignmentComponent::MarkPackageDirty()
{
#if WITH_EDITOR
	if (!ItemId.IsValid())
	{
		ItemId = FGuid::NewGuid();
		ProtectedItemId = ItemId;
	}

	Modify();
	if (UPackage* Package = GetOutermost())
		Package->MarkPackageDirty();
	if (AActor* Owner = GetOwner())
		Owner->MarkPackageDirty();
#endif
}

void UFloorAssignmentComponent::PublishRegistration()
{
	if (SnapshotChannel != ESnapshotChannel::Snapshot || !ItemId.IsValid())
		return;

	UWorld* W = GetWorld();
	if (!W) return;

	UGameInstance* GI = W->GetGameInstance();
	if (!GI) return;

	UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>();
	if (!Bus) return;

	UFloorPlacementPayload* Payload = Bus->CreatePayload<UFloorPlacementPayload>();
	if (!Payload) return;

	AActor* Owner = GetOwner();
	TArray<FName> TextTags;
	if (Owner)
		TextTags = Owner->Tags;

	Payload->SetupWithTags(
		ActorType,
		Owner ? Owner->GetActorTransform() : FTransform::Identity,
		ItemId,
		Owner ? Owner->GetClass() : nullptr,
		this->GameplayTagContainer,
		TextTags,
		true
	);

	IsRuntimeSpawned = true;
	FOutcomeEventBase Ev;
	Ev.OutcomeType = EOutcomeType::Interior;
	Ev.OutcomeInterior = EOutcomeInterior::FloorPlacementRegistered;
	Ev.Payload = Payload;
	Bus->PublishOutcome(Ev);
}

void UFloorAssignmentComponent::Registrate(EFloorActorType Type, FGuid ForceItemId /*= FGuid()*/)
{
	AutoFillFloorIdsFromWorld();

	ItemId = ForceItemId.IsValid() ? ForceItemId : ItemId.IsValid() ? ItemId : FGuid::NewGuid();
	ProtectedItemId = ItemId;
	SnapshotChannel = ESnapshotChannel::Snapshot;
	ActorType = Type;
	MarkPackageDirty();

	PublishRegistration();

	if (ItemId.IsValid())
	{
		if (UInteriorSubsystem* Subsystem = GetInteriorSubsystem())
		{
			Subsystem->RegisterFloorActor(GetOwner(), ItemId);
		}
	}
}

void UFloorAssignmentComponent::AutoFillFloorIdsFromWorld()
{
	UWorld* World = GetWorld();
	if (!World) return;

#if WITH_EDITOR
	// ─── Редактор: источник данных — только ассеты, якоря на сцене игнорируем ──
	const FString CurrentPackage = World->GetOutermost()->GetName();
	if (CurrentPackage.IsEmpty()) return;

	const FString NormCurrent = NormalizeLevelName_FA(CurrentPackage);

	FAssetRegistryModule& ARM =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	Filter.ClassPaths.Add(UFloorAsset::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> FloorAssets;
	ARM.Get().GetAssets(Filter, FloorAssets);

	for (const FAssetData& AD : FloorAssets)
	{
		UFloorAsset* Floor = Cast<UFloorAsset>(AD.GetAsset());
		if (!Floor) Floor = Cast<UFloorAsset>(AD.ToSoftObjectPath().TryLoad());
		if (!Floor || Floor->FloorLevel.IsNull()) continue;

		const FString FloorPkg = Floor->FloorLevel.ToSoftObjectPath().GetLongPackageName();
		if (NormalizeLevelName_FA(FloorPkg) == NormCurrent)
		{
			ApplyFloorToComponent_FA(this, Floor);
			return;
		}
	}
#else
	// ─── Рантайм: AssetRegistry недоступен (Shipping), используем якорь на сцене ──
	for (TActorIterator<ALocationAnchorActor> It(World); It; ++It)
	{
		ALocationAnchorActor* Anchor = *It;
		if (!IsValid(Anchor)) continue;

		if (UFloorAsset* Floor = Anchor->OwnerFloor.LoadSynchronous())
		{
			ApplyFloorToComponent_FA(this, Floor);
			return;
		}
	}
#endif
}

#if WITH_EDITOR
void UFloorAssignmentComponent::AutoFillFromCurrentLevel()
{
	AutoFillFloorIdsFromWorld();
	MarkPackageDirty();
}

void UFloorAssignmentComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName = PropertyChangedEvent.GetPropertyName();

	// Триггерим, когда пользователь ставит SnapshotChannel=Snapshot
	// (это означает «зарегистрировать актора на этом уровне»).
	if (PropName == GET_MEMBER_NAME_CHECKED(UFloorAssignmentComponent, SnapshotChannel))
	{
		if (SnapshotChannel == ESnapshotChannel::Snapshot)
		{
			AutoFillFloorIdsFromWorld();
		}
	}
}
#endif
