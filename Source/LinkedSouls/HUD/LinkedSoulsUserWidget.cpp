#include "HUD/LinkedSoulsUserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Engine.h"

ULinkedSoulsUserWidget::ULinkedSoulsUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULinkedSoulsUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!HealthBar && !SoulEnergyBar && !CorruptionBar)
	{
		BuildHUD();
	}

	SetVisibility(ESlateVisibility::Visible);

	if (SynergyIndicator)
	{
		SynergyIndicator->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (WorldIndicator)
	{
		WorldIndicator->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (HealthLabel)
	{
		HealthLabel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SoulEnergyLabel)
	{
		SoulEnergyLabel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CorruptionLabel)
	{
		CorruptionLabel->SetVisibility(ESlateVisibility::Collapsed);
	}

	UpdateHealthBar(CurrentHealth, MaxHealth);
	UpdateSoulEnergyBar(CurrentSoulEnergy, MaxSoulEnergy);
	UpdateCorruptionBar(CurrentCorruption, MaxCorruption);
}

static UProgressBar* CreateBar(UWidgetTree* Tree, const FLinearColor& FillColor)
{
	UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());

	FProgressBarStyle Style;

	FSlateBrush BgBrush;
	BgBrush.DrawAs = ESlateBrushDrawType::Box;
	BgBrush.TintColor = FLinearColor(0.08f, 0.08f, 0.08f, 0.85f);
	Style.SetBackgroundImage(BgBrush);

	FSlateBrush FillBrush;
	FillBrush.DrawAs = ESlateBrushDrawType::Box;
	FillBrush.TintColor = FillColor;
	Style.SetFillImage(FillBrush);

	Bar->SetWidgetStyle(Style);
	Bar->SetPercent(1.0f);
	Bar->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));

	return Bar;
}

static UTextBlock* CreateLabel(UWidgetTree* Tree, const FString& Text)
{
	UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(FText::FromString(Text));
	Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo FontInfo = Label->GetFont();
	FontInfo.Size = 13;
	FontInfo.OutlineSettings.OutlineSize = 1;
	FontInfo.OutlineSettings.OutlineColor = FLinearColor::Black;
	Label->SetFont(FontInfo);
	Label->SetJustification(ETextJustify::Center);
	return Label;
}

static UOverlay* MakeBarOverlay(UWidgetTree* Tree, UProgressBar* Bar, UTextBlock* Label)
{
	UOverlay* Overlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

	UOverlaySlot* BarSlot = Cast<UOverlaySlot>(Overlay->AddChild(Bar));
	BarSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
	BarSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);

	UOverlaySlot* LabelSlot = Cast<UOverlaySlot>(Overlay->AddChild(Label));
	LabelSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
	LabelSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);

	return Overlay;
}

void ULinkedSoulsUserWidget::BuildHUD()
{
	UWidgetTree* Tree = WidgetTree;
	if (!Tree)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red,
				TEXT("ULinkedSoulsUserWidget: WidgetTree is null!"));
		}
		return;
	}

	UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	Tree->RootWidget = Canvas;

	VBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Canvas->AddChild(VBox));
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		CanvasSlot->SetPosition(FVector2D(20.0f, 20.0f));
		CanvasSlot->SetSize(FVector2D(300.0f, 400.0f));
		CanvasSlot->SetAutoSize(true);
	}

	// ── Health ──
	HealthBar = CreateBar(Tree, FLinearColor(0.8f, 0.15f, 0.15f));
	HealthLabel = CreateLabel(Tree, TEXT("HP: 100 / 100"));

	UOverlay* HealthOverlay = MakeBarOverlay(Tree, HealthBar, HealthLabel);
	UVerticalBoxSlot* HealthVSlot = Cast<UVerticalBoxSlot>(VBox->AddChild(HealthOverlay));
	if (HealthVSlot)
	{
		HealthVSlot->SetPadding(FMargin(20.0f, 20.0f, 0.0f, 2.0f));
		HealthVSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// ── Soul Energy ──
	SoulEnergyBar = CreateBar(Tree, FLinearColor(0.15f, 0.45f, 0.9f));
	SoulEnergyLabel = CreateLabel(Tree, TEXT("Energy: 50 / 100"));

	UOverlay* EnergyOverlay = MakeBarOverlay(Tree, SoulEnergyBar, SoulEnergyLabel);
	UVerticalBoxSlot* EnergyVSlot = Cast<UVerticalBoxSlot>(VBox->AddChild(EnergyOverlay));
	if (EnergyVSlot)
	{
		EnergyVSlot->SetPadding(FMargin(20.0f, 2.0f, 0.0f, 2.0f));
		EnergyVSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// ── Corruption ──
	CorruptionBar = CreateBar(Tree, FLinearColor(0.6f, 0.1f, 0.7f));
	CorruptionLabel = CreateLabel(Tree, TEXT("Corruption: 0 / 100"));

	UOverlay* CorruptOverlay = MakeBarOverlay(Tree, CorruptionBar, CorruptionLabel);
	UVerticalBoxSlot* CorruptVSlot = Cast<UVerticalBoxSlot>(VBox->AddChild(CorruptOverlay));
	if (CorruptVSlot)
	{
		CorruptVSlot->SetPadding(FMargin(20.0f, 2.0f, 0.0f, 2.0f));
		CorruptVSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	// ── Indicators (synergy + world) ──
	SynergyIndicator = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FLinearColor(1.0f, 0.85f, 0.0f);
		SynergyIndicator->SetBrush(Brush);
		SynergyIndicator->SetDesiredSizeOverride(FVector2D(28.0f, 28.0f));
		SynergyIndicator->SetVisibility(ESlateVisibility::Collapsed);
	}
	UVerticalBoxSlot* SynergyVSlot = Cast<UVerticalBoxSlot>(VBox->AddChild(SynergyIndicator));
	if (SynergyVSlot)
	{
		SynergyVSlot->SetPadding(FMargin(20.0f, 6.0f, 0.0f, 2.0f));
		SynergyVSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	WorldIndicator = Tree->ConstructWidget<UImage>(UImage::StaticClass());
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FLinearColor(0.2f, 0.8f, 1.0f, 0.7f);
		WorldIndicator->SetBrush(Brush);
		WorldIndicator->SetDesiredSizeOverride(FVector2D(28.0f, 28.0f));
		WorldIndicator->SetVisibility(ESlateVisibility::Collapsed);
	}
	UVerticalBoxSlot* WorldVSlot = Cast<UVerticalBoxSlot>(VBox->AddChild(WorldIndicator));
	if (WorldVSlot)
	{
		WorldVSlot->SetPadding(FMargin(20.0f, 2.0f, 0.0f, 2.0f));
		WorldVSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
			TEXT("HUD: Layout built"));
	}
}

// ── Update functions ─────────────────────────────────────────────────────

void ULinkedSoulsUserWidget::UpdateHealthBar(float Current, float Max)
{
	CurrentHealth = Current;
	MaxHealth = Max;

	if (HealthBar)
	{
		HealthBar->SetPercent(Max > 0.0f ? Current / Max : 0.0f);
	}
	if (HealthLabel)
	{
		HealthLabel->SetText(FText::FromString(
			FString::Printf(TEXT("HP: %.0f / %.0f"), Current, Max)));
	}

	OnHealthUpdated(Current, Max);
}

void ULinkedSoulsUserWidget::UpdateSoulEnergyBar(float Current, float Max)
{
	CurrentSoulEnergy = Current;
	MaxSoulEnergy = Max;

	if (SoulEnergyBar)
	{
		SoulEnergyBar->SetPercent(Max > 0.0f ? Current / Max : 0.0f);
	}
	if (SoulEnergyLabel)
	{
		SoulEnergyLabel->SetText(FText::FromString(
			FString::Printf(TEXT("Energy: %.0f / %.0f"), Current, Max)));
	}

	OnSoulEnergyUpdated(Current, Max);
}

void ULinkedSoulsUserWidget::UpdateCorruptionBar(float Current, float Max)
{
	CurrentCorruption = Current;
	MaxCorruption = Max;

	if (CorruptionBar)
	{
		CorruptionBar->SetPercent(Max > 0.0f ? Current / Max : 0.0f);
	}
	if (CorruptionLabel)
	{
		CorruptionLabel->SetText(FText::FromString(
			FString::Printf(TEXT("Corruption: %.0f / %.0f"), Current, Max)));
	}

	OnCorruptionUpdated(Current, Max);
}

void ULinkedSoulsUserWidget::SetEnergyDepletedWarning(bool bDepleted)
{
	bEnergyDepletedWarning = bDepleted;
	OnEnergyWarningChanged(bDepleted);
}

void ULinkedSoulsUserWidget::UpdateElementSlot(int32 SlotIndex, ELinkedSoulsElement Element)
{
	if (SlotIndex == 0)
	{
		ActiveElementSlot0 = Element;
	}
	else if (SlotIndex == 1)
	{
		ActiveElementSlot1 = Element;
	}
	OnElementSlotUpdated(SlotIndex, Element);
}

void ULinkedSoulsUserWidget::UpdateWorldIndicator(bool bInSpiritWorld)
{
	if (WorldIndicator)
	{
		WorldIndicator->SetVisibility(
			bInSpiritWorld ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	OnWorldIndicatorUpdated(bInSpiritWorld);
}

void ULinkedSoulsUserWidget::SetSynergyActive(bool bActive)
{
	if (SynergyIndicator)
	{
		SynergyIndicator->SetVisibility(
			bActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}
