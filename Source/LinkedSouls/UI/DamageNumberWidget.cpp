// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/DamageNumberWidget.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"
#include "UObject/Object.h"

UDamageNumberWidget::UDamageNumberWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// bHasScriptImplementedTick defaults to true in UUserWidget, so NativeTick
	// is already enabled — no extra setup needed.
}

void UDamageNumberWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureText();
}

void UDamageNumberWidget::EnsureText()
{
	if (DamageText || !WidgetTree)
	{
		return;
	}

	// SizeBox root so we can offset the text upward via RenderTransform.
	USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Root->SetWidthOverride(120.f);
	Root->SetHeightOverride(30.f);
	WidgetTree->RootWidget = Root;

	DamageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (DamageText)
	{
		// Large readout so it pops at world-space scale.
		FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 28);
		DamageText->SetFont(Font);
		DamageText->SetJustification(ETextJustify::Center);
		Root->AddChild(DamageText);
	}
}

void UDamageNumberWidget::SetDamageValue(float Amount, bool bIsCorruption)
{
	EnsureText();
	if (!DamageText)
	{
		return;
	}

	const int32 Rounded = FMath::RoundToInt(Amount);
	if (bIsCorruption)
	{
		// purple "+" for corruption applied to the Soul
		DamageText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Rounded)));
		DamageText->SetColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.25f, 0.95f, 1.0f)));
	}
	else
	{
		// red "-" for damage dealt
		DamageText->SetText(FText::FromString(FString::Printf(TEXT("-%d"), Rounded)));
		DamageText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.15f, 0.15f, 1.0f)));
	}

	ElapsedTime = 0.0f;
}

void UDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);

	ElapsedTime += DeltaTime;

	if (!DamageText)
	{
		return;
	}

	// Rise phase: float upward over RiseDuration seconds.
	float RiseAlpha = 0.0f;
	if (RiseDuration > 0.0f)
	{
		RiseAlpha = FMath::Clamp(ElapsedTime / RiseDuration, 0.0f, 1.0f);
	}
	const float OffsetY = -RiseDistance * RiseAlpha;

	// Fade phase: begins after RiseDuration, opacity 1 -> 0 across the remainder.
	const float FadeDuration = FMath::Max(0.01f, LifetimeSeconds - RiseDuration);
	const float FadeAlpha = FMath::Clamp((ElapsedTime - RiseDuration) / FadeDuration, 0.0f, 1.0f);

	const FLinearColor BaseColor = DamageText->GetColorAndOpacity().GetSpecifiedColor();
	DamageText->SetColorAndOpacity(FSlateColor(FLinearColor(BaseColor.R, BaseColor.G, BaseColor.B, 1.0f - FadeAlpha)));

	// Apply the upward offset through the render transform translation.
	DamageText->SetRenderTranslation(FVector2D(0.0f, OffsetY));

	// Expire: tear down the hosting WidgetComponent + this widget.
	if (ElapsedTime >= LifetimeSeconds)
	{
		if (HostingComponent.IsValid())
		{
			// Destroying the WidgetComponent releases its widget reference and
			// the widget is garbage-collected — no explicit RemoveFromParent
			// needed (and RemoveFromParent logs "no UMG parent" for widgets
			// hosted in a WidgetComponent, since they have no UMG panel parent).
			HostingComponent->DestroyComponent();
		}
		else if (GetParent())
		{
			// Fallback: only detach if the widget actually has a UMG parent
			// (e.g. it was added to a panel instead of a WidgetComponent).
			RemoveFromParent();
		}
	}
}

void UDamageNumberWidget::SpawnAttached(TSubclassOf<UDamageNumberWidget> WidgetClass,
	AActor* AttachActor, float Amount, bool bIsCorruption)
{
	if (!WidgetClass || !AttachActor || !AttachActor->GetWorld())
	{
		return;
	}

	// Spawn a transient world-space WidgetComponent above the actor.
	UWidgetComponent* WidgetComp = NewObject<UWidgetComponent>(AttachActor);
	if (!WidgetComp)
	{
		return;
	}

	WidgetComp->SetWidgetClass(WidgetClass);
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComp->SetDrawSize(FVector2D(120.f, 30.f));
	WidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WidgetComp->SetVisibility(true);
	WidgetComp->SetupAttachment(AttachActor->GetRootComponent());
	// Float at roughly chest/upper-body height (80px) above the actor origin.
	WidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	WidgetComp->RegisterComponent();
	// Force the widget to construct synchronously so we can seed its value now.
	WidgetComp->InitWidget();

	if (UUserWidget* UserWidget = WidgetComp->GetUserWidgetObject())
	{
		if (UDamageNumberWidget* DamageWidget = Cast<UDamageNumberWidget>(UserWidget))
		{
			DamageWidget->HostingComponent = WidgetComp;
			DamageWidget->SetDamageValue(Amount, bIsCorruption);
		}
	}
}
