#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_FocusableButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClickedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPressedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReleasedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHoveredDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnhoveredDelegate);

class UButton;
class UTextBlock;

UCLASS()
class ALSEXTRAS_API UW_FocusableButton : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Button", meta = (BindWidget))
	UButton* Button;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	FButtonStyle Style;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	FLinearColor BackgroundColor;

	UPROPERTY(BlueprintReadWrite, Category = "Text", meta = (BindWidget))
	UTextBlock* TextBlock;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text|Appearance")
	FSlateFontInfo Font;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text|Appearance")
	FLinearColor TextColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text|Appearance")
	TEnumAsByte<ETextJustify::Type> Justification;

	UPROPERTY(BlueprintAssignable)
	FOnClickedDelegate OnClicked;

	UPROPERTY(BlueprintAssignable)
	FOnPressedDelegate OnPressed;

	UPROPERTY(BlueprintAssignable)
	FOnPressedDelegate OnReleased;

	UPROPERTY(BlueprintAssignable)
	FOnHoveredDelegate OnHovered;

	UPROPERTY(BlueprintAssignable)
	FOnUnhoveredDelegate OnUnhovered;

protected:
	virtual void NativeOnInitialized()override;

	virtual void NativePreConstruct()override;

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)override;

	UFUNCTION(BlueprintCallable)
	void HandleClicked();

	UFUNCTION(BlueprintCallable)
	void HandlePressed();

	UFUNCTION(BlueprintCallable)
	void HandleReleased();

	UFUNCTION(BlueprintCallable)
	void HandleHovered();

	UFUNCTION(BlueprintCallable)
	void HandleUnhovered();

public:
	UFUNCTION(BlueprintCallable)
	void RefreshButtonStyle();
};
