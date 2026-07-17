# Linked Souls

![Unreal Engine 5.8](https://img.shields.io/badge/Unreal%20Engine%205.8-000?style=flat&logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=flat&logo=cplusplus&logoColor=white)
![GAS](https://img.shields.io/badge/GAS-FF6F00?style=flat&logo=unrealengine&logoColor=white)
![AI](https://img.shields.io/badge/AI%20Module-BehaviorTree-2E7D32?style=flat)

## Game Overview

**Linked Souls** is a cooperative multiplayer game where two players are spiritually linked across dimensions. One player exists in the **physical world (Body)**, while the other inhabits the **spirit world (Soul)**. Together, they must cooperate to solve puzzles, defeat AI-driven enemies, and maintain their spiritual connection.

Core tech: **Gameplay Ability System (GAS)**, dual-world traversal, shared Soul Energy, co-op synergy, and **Behavior Tree enemy AI** with sight perception.

## Latest Updates (July 2026)

### Enemy AI (live)
- `ALinkedSoulsAIController` with **sight perception** (800cm / lose 1000cm)
- Runtime Behavior Tree graph: **Patrol → Chase → Attack**
- Blackboard keys: `TargetActor`, `HomeLocation`, `bCanSeeTarget`, `bIsAttacking`, …
- Assets: `/Game/AI/BB_LinkedSoulsEnemy`, `/Game/AI/BT_LinkedSoulsEnemy`
- NavMesh bounds on `RealWorld_Prototype`
- Enemy attacks **Soul** (+Corruption) and **Body** (−Health) via `PerformAttack`
- Floating enemy health bar (`UEnemyHealthBarWidget` + `UWidgetComponent`)

### Movement & mesh
- Camera-relative WASD (IMC double-swizzle fixed — no more sideways W)
- Manny mesh yaw **−90°** so character faces movement direction
- Soul: low gravity `0.3`, jump `150`, air control `0.8`

### HUD
- `WBP_LinkedSoulsHUD` bound to `ULinkedSoulsUserWidget`
- Health / Soul Energy / Corruption bars update from GAS + SoulEnergyComponent
- Labels & synergy/world indicators optional / collapsed by default

### Balance
| Setting | Old | New |
|---------|-----|-----|
| Corruption per enemy hit | +15 | **+8** |
| Soul Energy base regen | 2/s | **5/s** |
| Spirit Attack cost | 20 | **15** |
| Soul Pulse cost | 30 | **20** |
| Enemy attack cadence | 2s timer | **BT attack ~1.5s cooldown** |

### Input
- `IMC_Default` keyboard WASD cleared (avoids stacking with Body/Soul IMCs)
- Body/Soul IMCs at priority 2; correct Swizzle YXZ / Negate layout

---

## Features

- **Dual-World Gameplay** — Real (Body) and Spirit (Soul) dimensions
- **Co-op Multiplayer** — Listen Server, 2 players, partners linked on spawn
- **GAS Combat** — 7+ custom GameplayEffects, attribute death hooks
- **Shared Soul Energy** — one pool on GameState, regen/drain/ability costs
- **Co-op Synergy** — Body hits apply `Linked.Synergy.Active` buff to Soul (no re-apply spam)
- **Corruption** — Soul accumulates corruption; death at max
- **Enemy AI** — Perception + BT patrol/chase/attack (not a fixed timer)
- **World Portal** — dimension transition interactions
- **Pure C++ first** — UMG widgets + BB/BT assets where needed

## Controls

### Body — IMC_Body

| Action | Input | Description |
|--------|-------|-------------|
| Move | WASD | Camera-relative |
| Look | Mouse | Yaw / pitch |
| Jump | Space | Jump |
| Melee | F | Sphere sweep, physical damage, synergy on Soul |
| World Shift | E | Enter Spirit World (energy cost + drain) |

### Soul — IMC_Soul

| Action | Input | Description |
|--------|-------|-------------|
| Move | WASD | Camera-relative, low gravity |
| Look | Mouse | Yaw / pitch |
| Jump | Space | Jump (reduced Z velocity) |
| Spirit Attack | LMB | Line trace, costs **15** energy |
| Soul Pulse | Q | AoE sphere, costs **20** energy |
| Manifest | E | Enter Real World (energy cost + drain) |

## Combat (GAS)

| Effect | Amount | Notes |
|--------|--------|-------|
| GE_BodyMeleeDamage | −25 Health | Body melee |
| GE_SpiritAttackDamage | −30 Health | Soul LMB |
| GE_SpiritAttackSynergyDamage | −45 Health | With synergy tag |
| GE_SoulPulseDamage | −20 Health | AoE |
| GE_CorruptionDamage | **+8** Corruption | Enemy → Soul |
| GE_CorruptionDecay | −5 Corruption | Soul self, every 2s |
| GE_BodySynergyBuff | 10s | Tag `Linked.Synergy.Active` |

### Soul Energy

- Pool on `ALinkedSoulsGameState` / `USoulEnergyComponent`
- Start 50 / max 100
- Regen **5/s** base, higher when partners close
- Continuous drain during World Shift / Manifest

## Enemy AI Flow

```
Perception (sight)
    ↓
Blackboard TargetActor / bCanSeeTarget
    ↓
Behavior Tree (runtime graph if asset root empty)
    ├── Service: UpdateTarget (0.5s, clear after 3s lost sight)
    ├── Sequence: ChaseTarget → AttackTarget
    └── PatrolMove (random reachable / direct fallback)
```

- **Chase** acceptance ~150cm; pathfinding with **direct-move fallback**
- **Attack** range ~200cm, cooldown ~1.5s → `ABaseEnemy::PerformAttack`
- AI class: `ALinkedSoulsAIController` (AutoPossessAI on BaseEnemy)

## Project Structure

```
Source/LinkedSouls/
├── AI/                    # AIController, BT tasks/service, BB keys
├── Abilities/             # GAS GameplayEffects
├── Animation/             # LinkedSoulsAnimInstance
├── DualWorld/             # Dual world manager
├── Elements/              # ElementComponent
├── Enemies/               # BaseEnemy, TreeHeartBoss
├── GameState/             # LinkedSoulsGameState
├── HUD/                   # LinkedSoulsHUD + UserWidget
├── Player/                # Body, Soul, AttributeSet, base character
├── SoulEnergy/            # Shared energy component
├── UI/                    # EnemyHealthBarWidget
└── WorldPortal/           # Portal system

Content/
├── AI/                    # BB_LinkedSoulsEnemy, BT_LinkedSoulsEnemy
├── Blueprints/            # WBP_LinkedSoulsHUD, GameMode/GameState BPs
├── Input/                 # IMC_Body, IMC_Soul, IMC_Default, IA_*
└── Maps/                  # RealWorld_Prototype (+ NavMesh)
```

## Core Classes

| Class | Role |
|-------|------|
| `ALinkedSoulsGameMode` | Spawn Body/Soul, link partners |
| `ALinkedSoulsGameState` | Owns SoulEnergyComponent |
| `ABodyCharacter` / `ASoulCharacter` | Player pawns |
| `ALinkedSoulsPlayerCharacter` | Shared GAS, camera, movement |
| `ULinkedSoulsAttributeSet` | Health / SoulEnergy / Corruption |
| `ABaseEnemy` | Dual HP, PerformAttack, BT asset, health bar |
| `ALinkedSoulsAIController` | Perception + RunBehaviorTree |
| `UBTTask_PatrolMove` / `ChaseTarget` / `AttackTarget` | Combat AI tasks |
| `UBTService_UpdateTarget` | Clear target after lose-sight delay |
| `ALinkedSoulsHUD` / `ULinkedSoulsUserWidget` | Player HUD |
| `UEnemyHealthBarWidget` | World/screen enemy bar |

## Setup

### Prerequisites

- Unreal Engine **5.8**
- Visual Studio **2022** (C++)
- Git + **Git LFS**

### Install

```bash
git clone https://github.com/MatsoDev/LinkedSouls.git
cd LinkedSouls
git lfs install
git lfs pull
```

Generate VS project files from `LinkedSouls.uproject`, open solution, **Rebuild** `LinkedSoulsEditor`.

> Prefer full rebuild over Live Coding when constructors / new classes change.

### Multiplayer PIE

**Edit → Project Settings → Maps & Modes**  
Number of Players = **2**, Net Mode = **Play As Listen Server** → Play.

Expected logs when AI is healthy:
```
LinkedSoulsAI: Built runtime BT graph ...
LinkedSoulsAI: RunBehaviorTree OK
LinkedSouls AI: Target acquired [...]
BaseEnemy: Attacked [...]
```

## What's Next

Priority candidates (not started unless requested):

1. **Dual-world polish** — Real vs Spirit visibility, shift/manifest feel, portal co-op flow  
2. **Boss AI** — `TreeHeartBoss` phases, dual Physical/Spirit HP in BT  
3. **Combat juice** — attack anims, hit reacts, VFX, feedback  
4. **Enemy variety** — spawners, more archetypes, difficulty curves  
5. **Co-op puzzles** — Body+Soul required interactions  
6. **Replication pass** — listen-server / client edge cases  

---

**Built with Unreal Engine 5.8 · C++ · GAS · Enhanced Input · AIModule / Behavior Trees**

Repo: [github.com/MatsoDev/LinkedSouls](https://github.com/MatsoDev/LinkedSouls)
