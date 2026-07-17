#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHealthBarWidget.generated.h"

class UProgressBar;

UCLASS()
class LINKEDSOULS_API UEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateHealth(float Current, float Max);

	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* EnemyHealthBar;

private:
	void EnsureBar();
};
