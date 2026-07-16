#include "HUD/LinkedSoulsUserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

ULinkedSoulsUserWidget::ULinkedSoulsUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

// ── Helpers ──────────────────────────────────────────────────────────────

UProgressBar* ULinkedSoulsUserWidget::MakeBar(const FLinearColor& FillColor)
{
	UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
	FProgressBarStyle Style = Bar->GetWidgetStyle();
	FSlateBrush FillBrush;
	FillBrush.DrawAs = ESlateBrushDrawType::Box;
	FillBrush.TintColor = FillColor;
	Style.SetBackgroundImage(FSlateBrush());
	Style.SetFillImage(FillBrush);
	Bar->SetWidgetStyle(Style);
	Bar->SetPercent(1.0f);
	Bar->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	return Bar;
}

UTextBlock* ULinkedSoulsUserWidget::MakeLabel(const FString& DefaultText)
{
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(FText::FromString(DefaultText));
	Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	FSlateFontInfo FontInfo = Label->GetFont();
	FontInfo.Size = 14;
	Label->SetFont(FontInfo);
	return Label;
}

// ── Layout ───────────────────────────────────────────────────────────────

void ULinkedSoulsUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// If BindWidget slots came from a Blueprint child, use those instead
	if (!HealthBar && !SoulEnergyBar && !CorruptionBar)
	{
		CreateProgrammaticLayout();
	}
}

void ULinkedSoulsUserWidget::CreateProgrammaticLayout()
{
	// Root canvas — fills entire screen
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Root;

	// ── Health bar (top-left) ──
	HealthBar = MakeBar(FLinearColor(0.8f, 0.1f, 0.1f)); // red
	HealthLabel = MakeLabel(TEXT("HP: 100 / 100"));

	UCanvasPanelSlot* HealthSlot = Root->AddChildToCanvas(HealthBar);
	HealthSlot->SetAutoSize(true);
	HealthSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
	HealthSlot->SetPosition(FVector2D(20.0f, 20.0f));
	HealthSlot->SetSize(FVector2D(260.0f, 22.0f));

	UCanvasPanelSlot* HealthLabelSlot = Root->AddChildToCanvas(HealthLabel);
	HealthLabelSlot->SetAutoSize(true);
	HealthLabelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
	HealthLabelSlot->SetPosition(FVector2D(24.0f, 21.0f));

	// ── Soul Energy bar (top-center) ──
	SoulEnergyBar = MakeBar(FLinearColor(0.1f, 0.4f, 0.9f)); // blue
	SoulEnergyLabel = MakeLabel(TEXT("Energy: 50 / 100"));

	UCanvasPanelSlot* EnergySlot = Root->AddChildToCanvas(SoulEnergyBar);
	EnergySlot->SetAutoSize(true);
	EnergySlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
	EnergySlot->SetPosition(FVector2D(-130.0f, 20.0f));
	EnergySlot->SetSize(FVector2D(260.0f, 22.0f));

	UCanvasPanelSlot* EnergyLabelSlot = Root->AddChildToCanvas(SoulEnergyLabel);
	EnergyLabelSlot->SetAutoSize(true);
	EnergyLabelSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
	EnergyLabelSlot->SetPosition(FVector2D(-124.0f, 21.0f));

	// ── Corruption bar (top-right) — hidden for Body ──
	CorruptionBar = MakeBar(FLinearColor(0.6f, 0.1f, 0.7f)); // purple
	CorruptionLabel = MakeLabel(TEXT("Corruption: 0 / 100"));

	UCanvasPanelSlot* CorruptSlot = Root->AddChildToCanvas(CorruptionBar);
	CorruptSlot->SetAutoSize(true);
	CorruptSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
	CorruptSlot->SetPosition(FVector2D(-280.0f, 20.0f));
	CorruptSlot->SetSize(FVector2D(260.0f, 22.0f));

	UCanvasPanelSlot* CorruptLabelSlot = Root->AddChildToCanvas(CorruptionLabel);
	CorruptLabelSlot->SetAutoSize(true);
	CorruptLabelSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
	CorruptLabelSlot->SetPosition(FVector2D(-276.0f, 21.0f));

	// ── Synergy indicator ──
	SynergyIndicator = NewObject<UImage>(this);
	FSlateBrush SynBrush;
	SynBrush.DrawAs = ESlateBrushDrawType::Box;
	SynBrush.TintColor = FLinearColor(1.0f, 0.8f, 0.0f); // gold
	SynergyIndicator->SetBrush(SynBrush);
	SynergyIndicator->SetDesiredSizeOverride(FVector2D(32.0f, 32.0f));

	UCanvasPanelSlot* SynergySlot = Root->AddChildToCanvas(SynergyIndicator);
	SynergySlot->SetAutoSize(true);
	SynergySlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
	SynergySlot->SetPosition(FVector2D(100.0f, 20.0f));
	SynergySlot->SetSize(FVector2D(32.0f, 32.0f));

	// ── World indicator ──
	WorldIndicator = NewObject<UImage>(this);
	FSlateBrush WorldBrush;
	WorldBrush.DrawAs = ESlateBrushDrawType::Box;
	WorldBrush.TintColor = FLinearColor(0.2f, 0.8f, 1.0f, 0.6f); // cyan
	WorldIndicator->SetBrush(WorldBrush);
	WorldIndicator->SetDesiredSizeOverride(FVector2D(32.0f, 32.0f));

	UCanvasPanelSlot* WorldSlot = Root->AddChildToCanvas(WorldIndicator);
	WorldSlot->SetAutoSize(true);
	WorldSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
	WorldSlot->SetPosition(FVector2D(140.0f, 20.0f));
	WorldSlot->SetSize(FVector2D(32.0f, 32.0f));
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
		HealthLabel->SetText(FText::FromString(FString::Printf(TEXT("HP: %.0f / %.0f"), Current, Max)));
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
		SoulEnergyLabel->SetText(FText::FromString(FString::Printf(TEXT("Energy: %.0f / %.0f"), Current, Max)));
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
		CorruptionLabel->SetText(FText::FromString(FString::Printf(TEXT("Corruption: %.0f / %.0f"), Current, Max)));
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
		WorldIndicator->SetVisibility(bInSpiritWorld ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	OnWorldIndicatorUpdated(bInSpiritWorld);
}

void ULinkedSoulsUserWidget::SetSynergyActive(bool bActive)
{
	if (SynergyIndicator)
	{
		SynergyIndicator->SetVisibility(bActive ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}
