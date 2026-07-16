# Linked Souls

![Unreal Engine 5.8](https://img.shields.io/badge/Unreal%20Engine%205.8-000?style=flat&logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=flat&logo=cplusplus&logoColor=white)
![GAS](https://img.shields.io/badge/GAS-FF6F00?style=flat&logo=unrealengine&logoColor=white)

## Game Overview

**Linked Souls** is a cooperative multiplayer game where two players are spiritually linked across dimensions. One player exists in the **physical world (Body)**, while the other inhabits the **spirit world (Soul)**. Together, they must cooperate to solve intricate puzzles, defeat powerful enemies, and maintain their spiritual connection.

The core mechanics revolve around **Gameplay Ability System (GAS)**, dual-world traversal, shared Soul Energy management, and co-op synergy between Body and Soul.

## Features

- **Dual-World Gameplay**: Two distinct dimensions (Physical and Spirit) with unique mechanics
- **Cooperative Multiplayer**: Seamless 2-player experience via Listen Server
- **GAS Combat System**: Full GameplayAbilitySystem integration with 7 custom GameplayEffects
- **Shared Soul Energy Pool**: Both players consume from a single energy resource
- **Co-op Synergy Buff**: Body hits grant Soul bonus damage (Linked.Synergy.Active tag)
- **Corruption Mechanic**: Soul takes corruption damage over time, death at max corruption
- **Dynamic Combat**: Sphere-sweep melee (Body), line-trace spirit attack (Soul), AoE soul pulse
- **Enemy GAS**: Enemies own ASC + AttributeSet with their own corruption attacks
- **Pure C++ HUD**: UMG bars for Health, Soul Energy, Corruption with synergy/world indicators
- **Animation System**: Manny ABP assigned in C++ with custom AnimInstance properties
- **Enhanced Input System**: Separate IMC_Body / IMC_Soul mapping contexts

## Controls

### Body Player (Physical World) — IMC_Body

| Action | Input | Description |
|--------|-------|-------------|
| Movement | WASD | Camera-relative movement |
| Look Around | Mouse | Yaw/pitch camera control |
| Jump | Spacebar | Standard jump |
| Body Melee | F | 600cm sphere sweep, 25 damage, applies synergy buff to Soul |
| World Shift | E | Shift to Spirit World (costs energy, continuous drain) |

### Soul Player (Spirit World) — IMC_Soul

| Action | Input | Description |
|--------|-------|-------------|
| Movement | WASD | Camera-relative movement (low gravity 0.3f) |
| Look Around | Mouse | Yaw/pitch camera control |
| Jump | Spacebar | Standard jump |
| Spirit Attack | LMB | 800cm line trace, 30 damage (45 with synergy), costs 20 energy |
| Soul Pulse | Q | 400cm AoE sphere, 20 damage all targets, costs 30 energy |
| Manifest | E | Manifest to Real World (costs energy, continuous drain) |

## Combat System (GAS)

### Gameplay Effects

| Effect | Type | Amount | Tags |
|--------|------|--------|------|
| GE_BodyMeleeDamage | Instant | -25 Health | `Combat.Damage.Physical` |
| GE_SpiritAttackDamage | Instant | -30 Health | `Combat.Damage.Spirit` |
| GE_SpiritAttackSynergyDamage | Instant | -45 Health | `Combat.Damage.Spirit` |
| GE_SoulPulseDamage | Instant | -20 Health | `Combat.Damage.Spirit`, `Combat.AoE` |
| GE_CorruptionDamage | Instant | +15 Corruption | `Combat.Damage.Corruption` |
| GE_CorruptionDecay | Instant | -5 Corruption | (self-applied every 2s) |
| GE_BodySynergyBuff | Duration 10s | — | `Linked.Synergy.Active` |

### Hit Detection

- **Body Melee (F)**: Server RPC → `SweepMultiByChannel(ECC_Pawn)` with 600cm length, 100cm radius sphere. Applies GE_BodyMeleeDamage to each target with an ASC. On any hit, applies GE_BodySynergyBuff to linked Soul's ASC.
- **Spirit Attack (LMB)**: Server RPC → `LineTraceSingleByChannel(ECC_Pawn)` 800cm. Applies GE_SpiritAttackDamage or GE_SpiritAttackSynergyDamage (if synergy tag present). Costs 20 Soul Energy.
- **Soul Pulse (Q)**: Server RPC → `OverlapMultiByObjectType(ECC_Pawn)` 400cm radius. Applies GE_SoulPulseDamage to all targets. Costs 30 Soul Energy.
- **Enemy Attack**: `OverlapMultiByObjectType(ECC_Pawn)` 200cm radius every 2s. Applies GE_CorruptionDamage (+15 Corruption) to nearby Soul.

### Death Handling

- `PostGameplayEffectExecute` in `ULinkedSoulsAttributeSet` checks Health ≤ 0 (Body) or Corruption ≥ Max (Soul)
- On death: `Owner->Destroy()` or `OnCharacterDeath()` for players; `SetLifeSpan(3.0f)` for enemies

### Soul Energy System

- **Shared pool** on `ALinkedSoulsGameState` via `USoulEnergyComponent`
- Starts at 50, max 100
- **Regen**: +2/sec base, +5/sec when Body+Soul within 1000cm
- **Drain**: -2/sec while World Shift/Manifest active
- **Ability costs**: SpiritAttack 20, SoulPulse 30, Shift/Manifest 20 flat + continuous drain
- Blocked if insufficient energy (`ConsumeSoulEnergy` returns false)

## Project Structure

```
LinkedSouls/
├── Source/LinkedSouls/
│   ├── Abilities/                # 7 GAS GameplayEffects (.h/.cpp pair each)
│   │   ├── GE_BodyMeleeDamage.h/cpp
│   │   ├── GE_BodySynergyBuff.h/cpp
│   │   ├── GE_CorruptionDamage.h/cpp
│   │   ├── GE_CorruptionDecay.h/cpp
│   │   ├── GE_SoulPulseDamage.h/cpp
│   │   ├── GE_SpiritAttackDamage.h/cpp
│   │   └── GE_SpiritAttackSynergyDamage.h/cpp
│   ├── Animation/                # C++ AnimInstance (Speed, bIsInAir, bIsAttacking)
│   ├── DualWorld/                # World dimension manager
│   ├── Elements/                 # ElementComponent + ELinkedSoulsElement
│   ├── Enemies/                  # ABaseEnemy (GAS, dual-HP, corruption attacks)
│   ├── GameState/                # ALinkedSoulsGameState (owns SoulEnergyComponent)
│   ├── HUD/                      # ALinkedSoulsHUD + ULinkedSoulsUserWidget
│   ├── Input/                    # Enhanced Input configuration
│   ├── Player/                   # BodyCharacter, SoulCharacter, AttributeSet
│   ├── Puzzles/                  # Puzzle mechanics
│   ├── SoulEnergy/               # USoulEnergyComponent (shared pool)
│   └── WorldPortal/              # Dimension transition system
├── Content/
│   ├── Blueprints/               # BP_LinkedSoulsGameMode
│   ├── Characters/               # Manny/SK_Mannequin meshes + ABP_Manny
│   ├── Input/                    # IMC_Body, IMC_Soul, IA_* assets
│   └── Maps/                     # Game levels
├── Config/
├── LinkedSouls.uproject
└── README.md
```

## Core Classes

| Class | File | Description |
|-------|------|-------------|
| `ALinkedSoulsGameMode` | `LinkedSoulsGameMode.h/cpp` | Spawns Body/Soul in PostLogin, links partners |
| `ALinkedSoulsGameState` | `GameState/LinkedSoulsGameState.h/cpp` | Owns the shared SoulEnergyComponent |
| `ABodyCharacter` | `Player/BodyCharacter.h/cpp` | Body player — melee combat, World Shift |
| `ASoulCharacter` | `Player/SoulCharacter.h/cpp` | Soul player — spirit attacks, corruption decay |
| `ALinkedSoulsPlayerCharacter` | `Player/LinkedSoulsPlayerCharacter.h/cpp` | Base class — GAS init, camera, input |
| `ULinkedSoulsAttributeSet` | `Player/LinkedSoulsAttributeSet.h/cpp` | Health, SoulEnergy, Corruption attributes |
| `USoulEnergyComponent` | `SoulEnergy/SoulEnergyComponent.h/cpp` | Shared energy pool with regen/drain |
| `ABaseEnemy` | `Enemies/BaseEnemy.h/cpp` | Enemy with GAS, dual HP, corruption attacks |
| `ALinkedSoulsHUD` | `HUD/LinkedSoulsHUD.h/cpp` | HUD manager — attribute/synergy binding |
| `ULinkedSoulsUserWidget` | `HUD/LinkedSoulsUserWidget.h/cpp` | C++ UMG widget with programmatic layout |
| `ULinkedSoulsAnimInstance` | `Animation/LinkedSoulsAnimInstance.h/cpp` | Animation blueprint properties |

## Setup & Installation

### Prerequisites

- **Unreal Engine 5.8**
- **Visual Studio 2022** with C++ support
- **Git** with LFS support

### Installation

```bash
git clone https://github.com/MatsoDev/LinkedSouls.git
cd LinkedSouls
git lfs install
git lfs pull
```

Right-click `LinkedSouls.uproject` → **Generate Visual Studio project files**, then build in Visual Studio.

### Testing Multiplayer

In Editor: **Edit → Project Settings → Maps & Modes** → set **Number of Players** to 2, **Net Mode** to `Play As Listen Server`. Press Play.

## Development

All systems are pure C++ — no Blueprint scripting required. Modify source in `Source/LinkedSouls/`, compile via **Live Coding** or rebuild in Visual Studio.

---

**Built with Unreal Engine 5.8 | C++ | Gameplay Ability System | Enhanced Input**
