#include "HUD/LinkedSoulsHUD.h"
#include "Player/BodyCharacter.h"
#include "Player/SoulCharacter.h"
#include "Player/LinkedSoulsPlayerCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"

ALinkedSoulsHUD::ALinkedSoulsHUD()
{
	static ConstructorHelpers::FClassFinder<ULinkedSoulsUserWidget> WidgetBPClass(
		TEXT("/Game/Blueprints/WBP_LinkedSoulsHUD.WBP_LinkedSoulsHUD_C"));
	if (WidgetBPClass.Succeeded())
	{
		HUDWidgetClass = WidgetBPClass.Class;
		UE_LOG(LogTemp, Warning, TEXT("LinkedSoulsHUD: WBP_LinkedSoulsHUD loaded ✓"));
	}
	else
	{
		HUDWidgetClass = ULinkedSoulsUserWidget::StaticClass();
		UE_LOG(LogTemp, Error, TEXT("LinkedSoulsHUD: WBP NOT FOUND — fallback to C++ widget"));
	}
}

void ALinkedSoulsHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!HUDWidgetClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("LinkedSoulsHUD: HUDWidgetClass not set — assign in GameMode or BP"));
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	HUDWidget = CreateWidget<ULinkedSoulsUserWidget>(PC, HUDWidgetClass);
	if (!HUDWidget)
	{
		UE_LOG(LogTemp, Error,
			TEXT("LinkedSoulsHUD: Failed to create widget from HUDWidgetClass"));
		return;
	}

	HUDWidget->SetVisibility(ESlateVisibility::Visible);
	HUDWidget->AddToViewport(0);
	UE_LOG(LogTemp, Warning, TEXT("LinkedSoulsHUD: Widget added ✓"));
	BindToLocalPlayer();
}

void ALinkedSoulsHUD::BindToLocalPlayer()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ALinkedSoulsHUD::BindToLocalPlayer);
		UE_LOG(LogTemp, Warning,
			TEXT("LinkedSoulsHUD: Local pawn is null — deferring bind"));
		return;
	}

	ALinkedSoulsPlayerCharacter* BaseChar =
		Cast<ALinkedSoulsPlayerCharacter>(Pawn);
	if (!BaseChar)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("LinkedSoulsHUD: Pawn is not a LinkedSoulsPlayerCharacter"));
		return;
	}

	// ── ASC attribute binding ──
	UAbilitySystemComponent* ASC =
		BaseChar->FindComponentByClass<UAbilitySystemComponent>();
	if (ASC)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(
			ULinkedSoulsAttributeSet::GetHealthAttribute())
			.AddUObject(this, &ALinkedSoulsHUD::OnHealthChanged);

		ASC->GetGameplayAttributeValueChangeDelegate(
			ULinkedSoulsAttributeSet::GetSoulEnergyAttribute())
			.AddUObject(this, &ALinkedSoulsHUD::OnSoulEnergyAttributeChanged);

		ASC->GetGameplayAttributeValueChangeDelegate(
			ULinkedSoulsAttributeSet::GetCorruptionAttribute())
			.AddUObject(this, &ALinkedSoulsHUD::OnCorruptionChanged);

		UE_LOG(LogTemp, Log,
			TEXT("LinkedSoulsHUD: ASC attribute delegates bound"));
	}

	// ── Body vs Soul determination ──
	if (Cast<ABodyCharacter>(Pawn))
	{
		bLocalPlayerIsBody = true;
		if (HUDWidget) HUDWidget->bIsBodyPlayer = true;
		UE_LOG(LogTemp, Log, TEXT("LinkedSoulsHUD: Local player = Body"));
	}
	else if (Cast<ASoulCharacter>(Pawn))
	{
		bLocalPlayerIsBody = false;
		if (HUDWidget) HUDWidget->bIsBodyPlayer = false;
		UE_LOG(LogTemp, Log, TEXT("LinkedSoulsHUD: Local player = Soul"));
	}

	// ── Initial bar values from ASC ──
	if (ASC && HUDWidget)
	{
		const float Health = ASC->GetNumericAttribute(ULinkedSoulsAttributeSet::GetHealthAttribute());
		const float MaxHealth = ASC->GetNumericAttribute(ULinkedSoulsAttributeSet::GetMaxHealthAttribute());
		const float SoulEnergy = ASC->GetNumericAttribute(ULinkedSoulsAttributeSet::GetSoulEnergyAttribute());
		const float MaxSoulEnergy = ASC->GetNumericAttribute(ULinkedSoulsAttributeSet::GetMaxSoulEnergyAttribute());
		const float Corruption = ASC->GetNumericAttribute(ULinkedSoulsAttributeSet::GetCorruptionAttribute());
		const float MaxCorruption = ASC->GetNumericAttribute(ULinkedSoulsAttributeSet::GetMaxCorruptionAttribute());

		if (bLocalPlayerIsBody)
		{
			HUDWidget->UpdateHealthBar(Health, MaxHealth);
		}
		else
		{
			HUDWidget->UpdateCorruptionBar(Corruption, MaxCorruption);
		}
		HUDWidget->UpdateSoulEnergyBar(SoulEnergy, MaxSoulEnergy);
	}

	// ── SoulEnergyComponent (on GameState) ──
	USoulEnergyComponent* SEC =
		USoulEnergyComponent::GetSoulEnergyComponent(this);
	if (SEC)
	{
		SEC->OnEnergyChanged.AddDynamic(
			this, &ALinkedSoulsHUD::OnSoulEnergyChanged);
		SEC->OnEnergyDepleted.AddDynamic(
			this, &ALinkedSoulsHUD::OnEnergyDepleted);
		SEC->OnEnergyFull.AddDynamic(
			this, &ALinkedSoulsHUD::OnEnergyFull);

		UE_LOG(LogTemp, Log,
			TEXT("LinkedSoulsHUD: SoulEnergyComponent delegates bound"));
	}

	// ── ElementComponent ──
	UElementComponent* ElemComp =
		Pawn->FindComponentByClass<UElementComponent>();
	if (ElemComp)
	{
		ElemComp->OnElementChanged.AddDynamic(
			this, &ALinkedSoulsHUD::OnActiveElementChanged);
		UE_LOG(LogTemp, Log,
			TEXT("LinkedSoulsHUD: ElementComponent delegate bound"));
	}

	// ── Synergy polling timer ──
	GetWorldTimerManager().SetTimer(SynergyPollTimer, this,
		&ALinkedSoulsHUD::PollSynergy, 0.5f, true);

	// fire an initial poll immediately
	PollSynergy();
}

// ────────────────────────────────────────────────────────────
// GAS Attribute Callbacks
// ────────────────────────────────────────────────────────────

void ALinkedSoulsHUD::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!bLocalPlayerIsBody) return;

	if (HUDWidget)
	{
		HUDWidget->UpdateHealthBar(Data.NewValue,
			GetAttributeMax(ULinkedSoulsAttributeSet::GetMaxHealthAttribute()));
	}
}

void ALinkedSoulsHUD::OnSoulEnergyAttributeChanged(
	const FOnAttributeChangeData& Data)
{
	if (HUDWidget)
	{
		HUDWidget->UpdateSoulEnergyBar(Data.NewValue,
			GetAttributeMax(ULinkedSoulsAttributeSet::GetMaxSoulEnergyAttribute()));
	}
}

void ALinkedSoulsHUD::OnCorruptionChanged(const FOnAttributeChangeData& Data)
{
	if (bLocalPlayerIsBody) return;

	if (HUDWidget)
	{
		HUDWidget->UpdateCorruptionBar(Data.NewValue,
			GetAttributeMax(ULinkedSoulsAttributeSet::GetMaxCorruptionAttribute()));
	}
}

// ────────────────────────────────────────────────────────────
// SoulEnergyComponent Callbacks
// ────────────────────────────────────────────────────────────

void ALinkedSoulsHUD::OnSoulEnergyChanged(float NewValue)
{
	if (HUDWidget)
	{
		USoulEnergyComponent* SEC =
			USoulEnergyComponent::GetSoulEnergyComponent(this);
		float Max = SEC ? SEC->MaxEnergy : 100.0f;
		HUDWidget->UpdateSoulEnergyBar(NewValue, Max);
	}
}

void ALinkedSoulsHUD::OnEnergyDepleted()
{
	if (HUDWidget)
	{
		HUDWidget->SetEnergyDepletedWarning(true);
	}
	UE_LOG(LogTemp, Log,
		TEXT("HUD: SoulEnergy DEPLETED warning shown"));
}

void ALinkedSoulsHUD::OnEnergyFull()
{
	if (HUDWidget)
	{
		HUDWidget->SetEnergyDepletedWarning(false);
	}
	UE_LOG(LogTemp, Log,
		TEXT("HUD: SoulEnergy full — warning cleared"));
}

// ────────────────────────────────────────────────────────────
// ElementComponent Callbacks
// ────────────────────────────────────────────────────────────

void ALinkedSoulsHUD::OnActiveElementChanged(ELinkedSoulsElement NewElement)
{
	if (HUDWidget)
	{
		HUDWidget->UpdateElementSlot(0, NewElement);
	}
	UE_LOG(LogTemp, Log, TEXT("HUD: Active element changed to %s"),
		*UEnum::GetValueAsString(NewElement));
}

// ────────────────────────────────────────────────────────────
// Synergy
// ────────────────────────────────────────────────────────────

void ALinkedSoulsHUD::PollSynergy()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	UAbilitySystemComponent* ASC =
		Pawn->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC) return;

	const bool bSynergyActive = ASC->HasMatchingGameplayTag(
		FGameplayTag::RequestGameplayTag(FName("Linked.Synergy.Active")));

	if (HUDWidget)
	{
		HUDWidget->SetSynergyActive(bSynergyActive);
	}
}

// ────────────────────────────────────────────────────────────
// Helpers
// ────────────────────────────────────────────────────────────

float ALinkedSoulsHUD::GetAttributeMax(FGameplayAttribute Attribute) const
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return 0.0f;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return 0.0f;

	UAbilitySystemComponent* ASC =
		Pawn->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC) return 0.0f;

	return ASC->GetNumericAttribute(Attribute);
}
