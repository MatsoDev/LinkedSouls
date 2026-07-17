#include "UI/EnemyHealthBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"

void UEnemyHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureBar();
	UpdateHealth(100.f, 100.f);
}

void UEnemyHealthBarWidget::EnsureBar()
{
	if (EnemyHealthBar || !WidgetTree)
	{
		return;
	}

	USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Root->SetWidthOverride(100.f);
	Root->SetHeightOverride(15.f);
	WidgetTree->RootWidget = Root;

	EnemyHealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());

	FProgressBarStyle Style;
	FSlateBrush BgBrush;
	BgBrush.DrawAs = ESlateBrushDrawType::Box;
	BgBrush.TintColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.85f);
	Style.SetBackgroundImage(BgBrush);

	FSlateBrush FillBrush;
	FillBrush.DrawAs = ESlateBrushDrawType::Box;
	FillBrush.TintColor = FLinearColor(0.85f, 0.1f, 0.1f, 1.f);
	Style.SetFillImage(FillBrush);

	EnemyHealthBar->SetWidgetStyle(Style);
	EnemyHealthBar->SetPercent(1.f);
	Root->AddChild(EnemyHealthBar);
}

void UEnemyHealthBarWidget::UpdateHealth(float Current, float Max)
{
	EnsureBar();
	if (EnemyHealthBar)
	{
		EnemyHealthBar->SetPercent(Max > 0.f ? Current / Max : 0.f);
	}
}
