#include "ArrayEffect/A_PythonContainer.h"
#include "ArrayEffect/A_ArrayNode.h"
#include "Kismet/GameplayStatics.h"
#include "AlsCharacterExample.h"
#include "AlsCameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"

AA_PythonContainer::AA_PythonContainer()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollisionComponent"));
	EndNodeComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("EndNodeComponent"));
	TextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRenderComponent"));

	RootComponent = SceneComponent;
	CollisionComponent->SetupAttachment(RootComponent);
	EndNodeComponent->SetupAttachment(RootComponent);
	TextComponent->SetupAttachment(RootComponent);
}

void AA_PythonContainer::BeginPlay()
{
	Super::BeginPlay();

	DefaultLocation = GetActorLocation();
	DefaultRotation = GetActorRotation();

	EndNodeComponent->DestroyComponent();
	EndNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, GetActorLocation(), GetActorRotation());
	if (EndNode)
	{
		EndNode->OwnerActor = this;

		FVector MinBound;
		FVector MaxBound;
		EndNode->NodeBorder->GetLocalBounds(MinBound, MaxBound);
		MinBound *= EndNode->NodeBorder->GetComponentScale();
		MaxBound *= EndNode->NodeBorder->GetComponentScale();
		NodeLength = MaxBound.X - MinBound.X;
		NodeWidth = MaxBound.Y - MinBound.Y;
		NodeHigh = MaxBound.Z - MinBound.Z;
	}

	CollisionComponent->SetBoxExtent(FVector(NodeLength * 0.5f, NodeWidth * 0.5f, NodeHigh * 0.5f));

	TextComponent->SetText(ContainerName);
}

void AA_PythonContainer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ContainerExtend();

	AttachToCharacterCamera();

	DetachFromCharacterCamera();

	AttachToContainer();

	RefreshNameLocationAndRotation();
}

void AA_PythonContainer::GetTextCommand(FText Command)
{
	if (bIsSwapping || EndNode->bIsMoving || bIsDetaching)
	{
		return;
	}

	int32 OutIndex = -1;

	int32 OutLeftIndex = -1;

	int32 OutRightIndex = -1;

	int32 SizeOfConcatenatedArray = -1;

	FText PrevName;

	FText NewName;

	FName VariableName = NAME_None;

	int32 ArrayNum;

	//append
	if (ParseCommandToAppend(Command, PrevName, VariableName) && PrevName.ToString() == ContainerName.ToString())
	{
		AppendNode(VariableName);
	}
	//pop
	else if (ParseCommandToPop(Command, PrevName, VariableName, OutIndex) && PrevName.ToString() == ContainerName.ToString())
	{
		if (VariableName.ToString() != "")
		{
			NodeArray[OutIndex]->GrabbedActor->Tags.AddUnique(VariableName);
		}
		ContainerPop(OutIndex);
		MoveNodesConsideringOrder();
	}
	//clear
	else if (ParseCommandToClear(Command, PrevName) && PrevName.ToString() == ContainerName.ToString())
	{
		ContainerClear();
		MoveNodesConsideringOrder();
	}
	//extend
	else if (ParseCommandToExtend(Command, PrevName, ExtendContainer) && PrevName.ToString() == ContainerName.ToString())
	{
	}
	//concatenate
	else if (ParseCommandToConcatenate(Command, SizeOfConcatenatingContainer, SizeOfConcatenatedArray))
	{
		if (NodeArray.Num() != SizeOfConcatenatedArray)
		{
			return;
		}

		bIsOnConcatenation = true;
	}
	//reset concatenation
	else if (bIsOnConcatenation && ParseCommandToReset(Command, PrevName) && PrevName.ToString() == ContainerName.ToString())
	{
		bIsOnConcatenation = false;
		bIsDetaching = true;;
	}
	//rename
	else if (ParseCommandToRename(Command, PrevName, NewName, ArrayNum) && PrevName.ToString() == ContainerName.ToString() && ArrayNum == NodeArray.Num())
	{
		ContainerRename(NewName);
	}
	//copy
	else if (ParseCommandToCopy(Command, PrevName, NewName, OutLeftIndex, OutRightIndex) && PrevName.ToString() == ContainerName.ToString())
	{
		ContainerCopy(NewName, OutLeftIndex, OutRightIndex);
	}
}

void AA_PythonContainer::AppendNode(FName VariableName)
{
	if (NodeArray.Num() == 10)
	{
		return;
	}

	EndNode->SetIndex(NodeArray.Num());

	if (!VariableName.IsNone())
	{
		if (AActor* GrabbedActor = GetActorWithTag(VariableName))
		{
			GrabbedActor->Tags.Remove(VariableName);
			EndNode->TryGrabActor(GrabbedActor);
		}
	}

	FVector SpawnLocation = EndNode->GetActorLocation();
	AA_ArrayNode* NewNode = EndNode;
	NodeArray.Add(NewNode);
	if (NodeClass)
	{
		EndNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, SpawnLocation, GetActorRotation());
		if (EndNode)
		{
			EndNode->OwnerActor = this;
			EndNode->MoveNode(EndNode->GetActorLocation() - GetActorRightVector() * NodeWidth);
		}
	}
}

void AA_PythonContainer::DeleteNode(int32 Index)
{
	if (NodeArray.IsEmpty())
	{
		return;
	}

	NodeArray[Index]->DeleteNode();

	for (size_t i = Index + 1; i < NodeArray.Num(); ++i)
	{
		NodeArray[i]->SetIndex(i - 1);
	}

	NodeArray.RemoveAt(Index);
}

void AA_PythonContainer::ContainerPop(int32 Index)
{
	DeleteNode(Index);
}

void AA_PythonContainer::ContainerClear()
{
	for (int32 i = NodeArray.Num() - 1; i >= 0; --i)
	{
		DeleteNode(i);
	}
}

void AA_PythonContainer::ContainerExtend()
{
	if (EndNode->bIsMoving)
	{
		return;
	}

	if (ExtendContainerIndex < ExtendContainer.Num())
	{
		++ExtendContainerIndex;
		AppendNode();
	}
	else
	{
		ExtendContainer.Empty();
		ExtendContainerIndex = 0;
	}
}

void AA_PythonContainer::ContainerConcatenate(AA_PythonContainer* ContainerToConcatenate)
{
	if (NodeArray.Num() != ContainerToConcatenate->SizeOfConcatenatingContainer)
	{
		ContainerToConcatenate->bIsOnConcatenation = false;
		ContainerToConcatenate->bIsDetaching = true;
		return;
	}

	ContainerToConcatenate->bIsOnConcatenation = false;

	int32 Index = NodeArray.Num();
	for (AA_ArrayNode* Node : ContainerToConcatenate->NodeArray)
	{
		Node->OwnerActor = this;
		Node->DefaultLocation = GetActorLocation() - GetActorRightVector() * NodeWidth * Index;
		Node->SetIndex(Index++);
	}

	ContainerToConcatenate->EndNode->Destroy();
	NodeArray.Append(ContainerToConcatenate->NodeArray);
	EndNode->DefaultLocation = NodeArray.Last()->DefaultLocation - GetActorRightVector() * NodeWidth;
	ContainerToConcatenate->Destroy();

	bIsAttaching = true;
}

void AA_PythonContainer::ContainerRename(FText NewName)
{
	SetContainerName(NewName);
}

void AA_PythonContainer::ContainerCopy(FText Name, int32 OutLeftIndex, int32 OutRightIndex)
{
	if (!ContainerClass || OutLeftIndex == -1 || OutRightIndex == -1)
	{
		return;
	}

	if (AA_PythonContainer* NewContainer = GetWorld()->SpawnActor<AA_PythonContainer>(ContainerClass, EndNode->DefaultLocation - GetActorRightVector() * NodeWidth * 2, GetActorRotation()))
	{
		NewContainer->DefaultLocation = NewContainer->GetActorLocation();
		NewContainer->SetContainerName(Name);

		int32 Index = 0;
		for (size_t i = OutLeftIndex; i < OutRightIndex; ++i)
		{
			if (AA_ArrayNode* NewNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, NodeArray[i]->GetActorLocation(), GetActorRotation()))
			{
				NewNode->OwnerActor = NewContainer;
				NewContainer->NodeArray.Add(NewNode);
				NewNode->SetIndex(Index++);
				if (NodeArray[i]->GrabbedActor)
				{
					if (AActor* CopyGrabbedActor = GetWorld()->SpawnActor<AActor>(NodeArray[i]->GrabbedActor->GetClass(), NewNode->GetActorLocation(), NewNode->GetActorRotation()))
					{
						NewNode->AttachToNode(CopyGrabbedActor);
						NewNode->GrabbedActor = CopyGrabbedActor;
						if (UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(CopyGrabbedActor->GetComponentByClass(UStaticMeshComponent::StaticClass())))
						{
							NewNode->GrabbedComponent = MeshComp;
						}
					}
				}
			}
		}
		NewContainer->MoveNodesConsideringOrder();
	}
}

bool AA_PythonContainer::IsValidPythonIdentifier(const FString& Str)
{
	if (Str.IsEmpty())
	{
		return false;
	}

	if (FChar::IsDigit(Str[0]))
	{
		return false;
	}

	for (TCHAR Ch : Str)
	{
		if (!(FChar::IsAlpha(Ch) || FChar::IsDigit(Ch) || Ch == TEXT('_')))
		{
			return false;
		}
	}

	static const TSet<FString> PythonKeywords =
	{
		TEXT("False"), TEXT("None"), TEXT("True"), TEXT("and"), TEXT("as"),
		TEXT("assert"), TEXT("break"), TEXT("class"), TEXT("continue"),
		TEXT("def"), TEXT("del"), TEXT("elif"), TEXT("else"), TEXT("except"),
		TEXT("finally"), TEXT("for"), TEXT("from"), TEXT("global"), TEXT("if"),
		TEXT("import"), TEXT("in"), TEXT("is"), TEXT("lambda"), TEXT("nonlocal"),
		TEXT("not"), TEXT("or"), TEXT("pass"), TEXT("raise"), TEXT("return"),
		TEXT("try"), TEXT("while"), TEXT("with"), TEXT("yield"),
		TEXT("list"), TEXT("dict"), TEXT("set"), TEXT("int"), TEXT("str")
	};

	if (PythonKeywords.Contains(Str))
	{
		return false;
	}

	return true;
}

bool AA_PythonContainer::ParseCommandToAppend(FText Command, FText& PrevName, FName& VariableName)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	FString ParsedContainerName, MethodPart;
	if (!Input.Split(TEXT("."), &ParsedContainerName, &MethodPart))
	{
		return false;

	}
	if (!IsValidPythonIdentifier(ParsedContainerName))
	{
		return false;
	}
	PrevName = FText::FromString(ParsedContainerName);

	const FString Prefix = TEXT("append(");
	const FString Suffix = TEXT(")");

	if (!MethodPart.StartsWith(Prefix) || !MethodPart.EndsWith(Suffix))
	{
		return false;
	}

	FString Inside = MethodPart.Mid(Prefix.Len(), MethodPart.Len() - Prefix.Len() - 1);
	if (!Inside.IsEmpty())
	{
		if (!IsValidPythonIdentifier(Inside))
		{
			return false;
		}
		VariableName = FName(*Inside);
	}

	return true;
}

bool AA_PythonContainer::ParseCommandToPop(FText Command, FText& PrevName, FName& VariableName, int32& Index)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	FString Left, Right;
	if (!Input.Split(TEXT("="), &Left, &Right))
	{
		Left.Empty();
		Right = Input;
	}

	if (!Left.IsEmpty())
	{
		if (!IsValidPythonIdentifier(Left))
		{
			return false;
		}
		VariableName = FName(Left);
	}

	const FString PopPrefix = TEXT(".pop(");
	const FString PopSuffix = TEXT(")");

	int32 PrefixPos = Right.Find(PopPrefix);
	if (PrefixPos == INDEX_NONE || !Right.EndsWith(PopSuffix))
	{
		return false;
	}

	FString ParsedContainerName = Right.Left(PrefixPos);
	if (!IsValidPythonIdentifier(ParsedContainerName))
	{
		return false;
	}
	PrevName = FText::FromString(ParsedContainerName);

	FString Inside = Right.Mid(PrefixPos + PopPrefix.Len(), Right.Len() - (PrefixPos + PopPrefix.Len() + 1));

	if (Inside.IsEmpty())
	{
		Index = NodeArray.Num() - 1;
	}
	else
	{
		if (!Inside.IsNumeric())
		{
			return false;
		}

		Index = FCString::Atoi(*Inside);
		if (Index < 0 || Index >= NodeArray.Num())
		{
			return false;
		}
	}

	return true;
}

bool AA_PythonContainer::ParseCommandToClear(FText Command, FText& PrevName)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	const FString Suffix = TEXT(".clear()");
	if (!Input.EndsWith(Suffix))
	{
		return false;
	}

	FString ParsedContainerName = Input.LeftChop(Suffix.Len());

	if (!IsValidPythonIdentifier(ParsedContainerName))
	{
		return false;
	}

	PrevName = FText::FromString(ParsedContainerName);
	return true;
}

bool AA_PythonContainer::ParseCommandToExtend(FText Command, FText& PrevName, TArray<int32>& OutArray)
{
	OutArray.Empty();
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	const FString Prefix = TEXT(".extend(");
	const FString Suffix = TEXT(")");

	int32 ExtendPos = Input.Find(Prefix);
	if (ExtendPos == INDEX_NONE || !Input.EndsWith(Suffix))
	{
		return false;
	}

	FString ParsedContainerName = Input.Left(ExtendPos);
	if (!IsValidPythonIdentifier(ParsedContainerName))
	{
		return false;
	}
	PrevName = FText::FromString(ParsedContainerName);

	FString Inner = Input.Mid(ExtendPos + Prefix.Len(), Input.Len() - (ExtendPos + Prefix.Len()) - Suffix.Len());

	TArray<FString> Parts;
	Inner.ParseIntoArray(Parts, TEXT(","), true);

	if (Parts.Num() == 0 && Inner.Len() > 0)
	{
		Parts.Add(Inner);
	}

	for (FString& Part : Parts)
	{
		if (!Part.StartsWith(TEXT("[")) || !Part.EndsWith(TEXT("]")))
		{
			OutArray.Empty();
			return false;
		}

		FString NumberStr = Part.Mid(1, Part.Len() - 2);

		if (!NumberStr.IsNumeric())
		{
			OutArray.Empty();
			return false;
		}

		OutArray.Add(FCString::Atoi(*NumberStr));
	}

	return true;
}

bool AA_PythonContainer::ParseCommandToConcatenate(FText Command, int32& OutSize1, int32& OutSize2)
{
	FString Input = Command.ToString();

	OutSize1 = 0;
	OutSize2 = 0;

	Input.RemoveSpacesInline();

	if (!Input.StartsWith(TEXT("arr=")))
		return false;

	int32 PlusIndex;
	if (!Input.FindChar('+', PlusIndex))
		return false;

	FString Left = Input.Mid(4, PlusIndex - 4);
	FString Right = Input.Mid(PlusIndex + 1);

	auto CountContainerElements = [](const FString& Part) -> int32
		{
			if (!Part.StartsWith(TEXT("[")) || !Part.EndsWith(TEXT("]")))
				return -1;

			FString Inner = Part.Mid(1, Part.Len() - 2);
			if (Inner.IsEmpty())
				return 0;

			TArray<FString> Elements;
			Inner.ParseIntoArray(Elements, TEXT(","), true);
			return Elements.Num();
		};

	OutSize1 = CountContainerElements(Left);
	OutSize2 = CountContainerElements(Right);

	return (OutSize1 > 0 && OutSize2 > 0 && (OutSize1 + OutSize2 <= 10));
}

bool AA_PythonContainer::ParseCommandToReset(FText Command, FText& PrevName)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	const FString Suffix = TEXT(".reset()");
	if (!Input.EndsWith(Suffix))
	{
		return false;
	}

	FString ParsedContainerName = Input.LeftChop(Suffix.Len());

	if (!IsValidPythonIdentifier(ParsedContainerName))
	{
		return false;
	}

	PrevName = FText::FromString(ParsedContainerName);
	return true;
}

bool AA_PythonContainer::ParseCommandToRename(FText Command, FText& PrevName, FText& NewName, int32& ArrayNum)
{
	const FString Input = Command.ToString();

	TArray<FString> Lines;
	Input.ParseIntoArrayLines(Lines);
	if (Lines.Num() != 3)
	{
		return false;
	}

	FString Left, Right;

	Lines[0].RemoveSpacesInline();
	if (!Lines[0].Split(TEXT("="), &Left, &Right) || Left.IsEmpty() || !Right.StartsWith(TEXT("[")) || !Right.EndsWith(TEXT("]")))
	{
		return false;
	}
	const FString ParsedContainerName = Left;
	FString Inner = Right.Mid(1, Right.Len() - 2);
	TArray<FString> Elements;
	Inner.ParseIntoArray(Elements, TEXT(","), true);
	ArrayNum = 0;
	for (const FString& Elem : Elements)
	{
		if (Elem.IsEmpty() || !Elem.IsNumeric())
		{
			return false;
		}
		++ArrayNum;
	}

	Lines[1].RemoveSpacesInline();
	if (!Lines[1].Split(TEXT("="), &Left, &Right) || Left.IsEmpty() || Right.IsEmpty() || Right != ParsedContainerName)
	{
		return false;
	}
	if (!IsValidPythonIdentifier(Left))
	{
		return false;
	}
	NewName = FText::FromString(Left);

	if (!Lines[2].StartsWith(TEXT("del ")))
	{
		return false;
	}
	FString DelName = Lines[2].Mid(4).TrimStartAndEnd();
	if (DelName.IsEmpty() || DelName != ParsedContainerName)
	{
		return false;
	}
	PrevName = FText::FromString(DelName);

	return true;
}

bool AA_PythonContainer::ParseCommandToCopy(FText Command, FText& PrevName, FText& CopyName, int32& OutLeftIndex, int32& OutRightIndex)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();
	FString Left, Right;

	if (!Input.Split(TEXT("="), &Left, &Right))
	{
		return false;
	}

	if (Left.IsEmpty() || Right.IsEmpty())
	{
		return false;
	}

	FString BaseName;
	OutLeftIndex = -1;
	OutRightIndex = -1;
	int32 OpenBracket, CloseBracket;

	if (Right.EndsWith(TEXT(".copy()")))
	{
		BaseName = Right.LeftChop(7);
		OutLeftIndex = 0;
		OutRightIndex = NodeArray.Num();
	}
	else if (Right.StartsWith(TEXT("list(")) && Right.EndsWith(TEXT(")")))
	{
		BaseName = Right.Mid(5, Right.Len() - 6);
		OutLeftIndex = 0;
		OutRightIndex = NodeArray.Num();
	}
	else if (Right.FindChar(TEXT('['), OpenBracket) && Right.FindLastChar(TEXT(']'), CloseBracket))
	{
		BaseName = Right.Left(OpenBracket);
		FString Inside = Right.Mid(OpenBracket + 1, CloseBracket - OpenBracket - 1);

		int32 ColonPos;
		if (!Inside.FindChar(TEXT(':'), ColonPos))
		{
			return false;
		}

		FString LeftSlice = Inside.Left(ColonPos);
		FString RightSlice = Inside.Mid(ColonPos + 1);

		if (LeftSlice.IsEmpty() && RightSlice.IsEmpty()) // [:]
		{
			OutLeftIndex = 0;
			OutRightIndex = NodeArray.Num();
		}
		else if (LeftSlice.IsEmpty() && RightSlice.IsNumeric()) // [:index]
		{
			OutLeftIndex = 0;
			OutRightIndex = FCString::Atoi(*RightSlice);
			if (OutRightIndex >= NodeArray.Num())
			{
				return false;
			}
		}
		else if (RightSlice.IsEmpty() && LeftSlice.IsNumeric()) // [index:]
		{
			OutLeftIndex = FCString::Atoi(*LeftSlice);
			OutRightIndex = NodeArray.Num();
			if (OutLeftIndex >= NodeArray.Num())
			{
				return false;
			}
		}
		else if (LeftSlice.IsNumeric() && RightSlice.IsNumeric()) // [index:index]
		{
			OutLeftIndex = FCString::Atoi(*LeftSlice);
			OutRightIndex = FCString::Atoi(*RightSlice);
			if (OutLeftIndex >= OutRightIndex || OutRightIndex >= NodeArray.Num())
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}

	if (!IsValidPythonIdentifier(Left) || !IsValidPythonIdentifier(BaseName))
	{
		return false;
	}

	if (Left == BaseName)
	{
		return false;
	}

	PrevName = FText::FromString(BaseName);
	CopyName = FText::FromString(Left);

	return true;
}

void AA_PythonContainer::AttachToCharacterCamera()
{
	if (!bIsOnConcatenation)
	{
		return;
	}

	if (AAlsCharacterExample* PC = Cast< AAlsCharacterExample>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		FMinimalViewInfo MinimalViewInfo;
		PC->CalcCamera(0.f, MinimalViewInfo);
		const FVector Location = MinimalViewInfo.Location;
		const FRotator Rotation = MinimalViewInfo.Rotation;
		const FVector Direction = Rotation.Vector();

		SetActorLocation(FMath::VInterpTo(GetActorLocation(), Location + Direction * 800.0f, GetWorld()->GetDeltaSeconds(), 2.0f));
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), (-Direction).Rotation(), GetWorld()->GetDeltaSeconds(), 2.0f));

		NodeArray[0]->SetActorLocation(FMath::VInterpTo(NodeArray[0]->GetActorLocation(), GetActorLocation(), GetWorld()->GetDeltaSeconds(), 2.0f));
		NodeArray[0]->SetActorRotation(FMath::RInterpTo(NodeArray[0]->GetActorRotation(), (-Direction).Rotation(), GetWorld()->GetDeltaSeconds(), 2.0f));

		for (int32 Index = 1; Index < NodeArray.Num(); ++Index)
		{
			NodeArray[Index]->SetActorLocation(FMath::VInterpTo(NodeArray[Index]->GetActorLocation(), NodeArray[Index - 1]->GetActorLocation() - NodeArray[Index - 1]->GetActorRightVector() * NodeWidth, GetWorld()->GetDeltaSeconds(), 2.0f));
			NodeArray[Index]->SetActorRotation(FMath::RInterpTo(NodeArray[Index]->GetActorRotation(), (-Direction).Rotation(), GetWorld()->GetDeltaSeconds(), 2.0f));
		}
		EndNode->SetActorLocation(FMath::VInterpTo(EndNode->GetActorLocation(), NodeArray.Last()->GetActorLocation() - NodeArray.Last()->GetActorRightVector() * NodeWidth, GetWorld()->GetDeltaSeconds(), 2.0f));
		EndNode->SetActorRotation(FMath::RInterpTo(EndNode->GetActorRotation(), (-Direction).Rotation(), GetWorld()->GetDeltaSeconds(), 2.0f));
	}
}

void AA_PythonContainer::DetachFromCharacterCamera()
{
	if (!bIsDetaching)
	{
		return;
	}

	if (GetActorLocation().Equals(DefaultLocation, 0.01f) && GetActorRotation().Equals(DefaultRotation, 0.01f))
	{
		bIsDetaching = false;
	}

	SetActorLocation(FMath::VInterpTo(GetActorLocation(), DefaultLocation, GetWorld()->GetDeltaSeconds(), 2.0f));
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), DefaultRotation, GetWorld()->GetDeltaSeconds(), 2.0f));
	for (AA_ArrayNode* Node : NodeArray)
	{
		Node->SetActorLocation(FMath::VInterpTo(Node->GetActorLocation(), Node->DefaultLocation, GetWorld()->GetDeltaSeconds(), 2.0f));
		Node->SetActorRotation(FMath::RInterpTo(Node->GetActorRotation(), DefaultRotation, GetWorld()->GetDeltaSeconds(), 2.0f));
	}
	EndNode->SetActorLocation(FMath::VInterpTo(EndNode->GetActorLocation(), EndNode->DefaultLocation, GetWorld()->GetDeltaSeconds(), 2.0f));
	EndNode->SetActorRotation(FMath::RInterpTo(EndNode->GetActorRotation(), DefaultRotation, GetWorld()->GetDeltaSeconds(), 2.0f));
}

void AA_PythonContainer::AttachToContainer()
{
	if (!bIsAttaching)
	{
		return;
	}

	if (EndNode->GetActorLocation().Equals(EndNode->DefaultLocation, 0.01f) && EndNode->GetActorRotation().Equals(DefaultRotation, 0.01f))
	{
		bIsAttaching = false;
	}

	for (AA_ArrayNode* Node : NodeArray)
	{
		Node->SetActorLocation(FMath::VInterpTo(Node->GetActorLocation(), Node->DefaultLocation, GetWorld()->GetDeltaSeconds(), 2.0f));
		Node->SetActorRotation(FMath::RInterpTo(Node->GetActorRotation(), DefaultRotation, GetWorld()->GetDeltaSeconds(), 2.0f));
	}
	EndNode->SetActorLocation(FMath::VInterpTo(EndNode->GetActorLocation(), EndNode->DefaultLocation, GetWorld()->GetDeltaSeconds(), 2.0f));
	EndNode->SetActorRotation(FMath::RInterpTo(EndNode->GetActorRotation(), DefaultRotation, GetWorld()->GetDeltaSeconds(), 2.0f));
}

void AA_PythonContainer::MoveNodesConsideringOrder()
{
	for (size_t i = 0; i < NodeArray.Num(); ++i)
	{
		NodeArray[i]->MoveNode(GetActorLocation() - GetActorRightVector() * NodeWidth * i);
	}
	if (IsValid(EndNode))
	{
		EndNode->MoveNode(GetActorLocation() - GetActorRightVector() * NodeWidth * NodeArray.Num());
	}
}

void AA_PythonContainer::RefreshNameLocationAndRotation()
{
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (!EndNode || !Player)
	{
		return;
	}

	FVector Start = GetActorLocation();
	FVector End = EndNode->GetActorLocation();
	//FVector LineDir = (End - Start).GetSafeNormal();
	//float Dot = FVector::DotProduct(Player->GetActorLocation() - Start, LineDir);
	//float ClampedDist = FMath::Clamp(Dot, 0.f, FVector::Dist(Start, End));
	//FVector ClampedPos = Start + LineDir * ClampedDist;
	//FVector PosToPlayerDir = Player->GetActorLocation() - ClampedPos;
	//PosToPlayerDir.Normalize();
	//ClampedPos += PosToPlayerDir * 100.0f;
	//ClampedPos.Z = GetActorLocation().Z - 140.0f + FMath::Sin(GetWorld()->GetTimeSeconds()) * 10.0f;
	//FVector NameLocation = FMath::VInterpTo(TextComponent->GetComponentLocation(), ClampedPos, GetWorld()->GetDeltaSeconds(), 2.0f);
	//TextComponent->SetWorldLocation(NameLocation);
	FVector NameLocation = (Start + End) * 0.5f;
	NameLocation.Z -= 140;
	TextComponent->SetWorldLocation(NameLocation);

	//FRotator NameRotation = (Player->GetActorLocation() - TextComponent->GetComponentLocation()).Rotation();
	//TextComponent->SetWorldRotation(NameRotation);
}

void AA_PythonContainer::SetContainerName(FText Name)
{
	ContainerName = Name;
	TextComponent->SetText(Name);
}

AActor* AA_PythonContainer::GetActorWithTag(const FName& Tag)
{
	if (!GetWorld())
	{
		return nullptr;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), Tag, FoundActors);

	return FoundActors.Num() > 0 ? FoundActors[0] : nullptr;
}