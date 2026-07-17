// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;
class UWidgetComponent;
class AActor;

/**
 *  Floating world-space damage number.
 *
 *  Spawned by ABaseEnemy (and the Soul on corruption) every time a hit lands.
 *  Rises 50px upward over 1.0s, then fades out over the next 0.5s, and removes
 *  itself (plus its hosting WidgetComponent) after 1.5s.
 *
 *  The float + fade is driven from NativeTick in pure C++ — Widget
 *  Animations (FWidgetAnimation) are editor-only constructs that cannot be
 *  authored from C++ in a packaged build, so we animate programmatically
 *  instead. This keeps the system pure-C++ (no Blueprint logic, per build rules).
 */
UCLASS()
class LINKEDSOULS_API UDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UDamageNumberWidget(const FObjectInitializer& ObjectInitializer);

	// ~Begin UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;
	// ~End UUserWidget interface

	/**
	 *  Sets the displayed damage value and colour.
	 *  @param Amount        Magnitude of the hit (positive number).
	 *  @param bIsCorruption Purple "+Amount" for corruption, red "-Amount" for damage.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Juice")
	void SetDamageValue(float Amount, bool bIsCorruption);

	/**
	 *  Spawns a world-space damage number attached to an actor.
	 *  Creates a transient UWidgetComponent at chest height, feeds it this
	 *  widget class, and lets NativeTick drive the rise + fade + cleanup.
	 *  @param WidgetClass    This widget subclass to instantiate.
	 *  @param AttachActor    Actor the number floats above (kept weak).
	 *  @param Amount         Damage magnitude to display.
	 *  @param bIsCorruption  Purple corruption vs red damage styling.
	 */
	static void SpawnAttached(TSubclassOf<UDamageNumberWidget> WidgetClass,
		AActor* AttachActor, float Amount, bool bIsCorruption);

private:

	/** Builds the TextBlock programmatically on first use (no BP asset needed). */
	void EnsureText();

	/** Text element that shows the damage value. Built in EnsureText(). */
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* DamageText;

	/** Time since spawn, used to drive the rise + fade timeline. */
	float ElapsedTime = 0.0f;

	/** Total lifetime before self-removal. */
	static constexpr float LifetimeSeconds = 1.5f;

	/** Distance (in slate units) the number floats upward. */
	static constexpr float RiseDistance = 50.0f;

	/** Duration of the upward float phase. */
	static constexpr float RiseDuration = 1.0f;

	/** Weak handle to the hosting WidgetComponent, torn down on expiry. */
	TWeakObjectPtr<UWidgetComponent> HostingComponent;
};
