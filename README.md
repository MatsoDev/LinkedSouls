# Linked Souls

![Unreal Engine 5](https://img.shields.io/badge/Unreal%20Engine%205-000?style=flat&logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=flat&logo=cplusplus&logoColor=white)
![Multiplayer](https://img.shields.io/badge/Multiplayer-00B4D8?style=flat&logo=unrealengine&logoColor=white)

## Game Overview

**Linked Souls** is a cooperative multiplayer game where two players are spiritually linked across dimensions. One player exists in the **physical world (Body)**, while the other inhabits the **spirit world (Soul)**. Together, they must cooperate to solve intricate puzzles, defeat powerful enemies, and maintain their spiritual connection to progress through a mysterious, interconnected world.

The core mechanic revolves around the **World Shift** ability, which allows players to transition between dimensions, revealing hidden paths and solving environmental puzzles that span both realms.

## Features

- **Dual-World Gameplay**: Two distinct dimensions (Physical and Spirit) with unique puzzles and mechanics
- **Cooperative Multiplayer**: Seamless 2-player experience via Listen Server
- **Spiritual Link System**: Players share abilities and must coordinate actions across dimensions
- **Dynamic Combat**: Spirit-based attacks that affect both physical and spiritual enemies
- **Puzzle Integration**: Environmental puzzles requiring both Body and Soul players to collaborate
- **Enhanced Input System**: Modern Unreal Engine 5 input handling with custom actions
- **Custom Game Architecture**: Dedicated GameMode, GameState, and Character classes for multiplayer
- **Progressive Difficulty**: Challenging encounters that test both players' coordination

## Controls

### Body Player (Physical World)

| Action | Input |
|--------|-------|
| Movement | WASD |
| Look Around | Mouse |
| Jump | Spacebar |
| Primary Attack | Left Mouse Button |
| Special Ability | Right Mouse Button |
| Interact | E |
| World Shift | Q |
| Sprint | Left Shift |

### Soul Player (Spirit World)

| Action | Input |
|--------|-------|
| Movement | WASD |
| Look Around | Mouse |
| Jump | Spacebar |
| Spirit Attack | Left Mouse Button |
| Soul Pulse | Right Mouse Button |
| Interact | E |
| World Shift | Q |
| Manifest | Left Shift |

## Project Structure

```
LinkedSouls/
├── Source/
│   ├── LinkedSouls.Target.cs
│   ├── LinkedSoulsEditor.Target.cs
│   └── LinkedSouls/
│       ├── Abilities/           # Gameplay ability system
│       ├── DualWorld/           # World dimension logic
│       ├── Elements/            # Environmental elements
│       ├── Enemies/             # Enemy AI and mechanics
│       ├── GameState/           # Game state management
│       ├── HUD/                 # User interface
│       ├── Input/               # Input system configuration
│       ├── Player/              # Player-specific code
│       ├── Puzzles/             # Puzzle mechanics
│       ├── SoulEnergy/          # Spirit world mechanics
│       ├── WorldPortal/         # Dimension transition system
│       └── LinkedSouls*.h/cpp   # Core game classes
├── Content/
│   ├── Blueprints/              # Visual scripting assets
│   ├── Characters/              # Character models and animations
│   ├── Input/                   # Input mapping contexts
│   ├── Maps/                    # Game levels
│   └── ...                      # Additional content directories
├── Config/                      # Engine configuration files
├── LinkedSouls.uproject         # Project file
└── README.md                    # This file
```

## Core Classes

| Class | File | Description |
|-------|------|-------------|
| `ALinkedSoulsGameMode` | `LinkedSoulsGameMode.h/cpp` | Custom GameMode for multiplayer setup |
| `ALinkedSoulsGameState` | `LinkedSoulsGameState.h/cpp` | Game state management and synchronization |
| `ALinkedSoulsCharacter` | `LinkedSoulsCharacter.h/cpp` | Main player character with dual-world capabilities |
| `ALinkedSoulsPlayerController` | `LinkedSoulsPlayerController.h/cpp` | Player input handling and network authority |

## Setup & Installation

### Prerequisites

- **Unreal Engine 5.3+** (recommended 5.4+)
- **Visual Studio 2022** or **Visual Studio Code** with C++ support
- **Git** with LFS support

### Installation Steps

1. **Clone the repository**
   ```bash
   git clone https://github.com/MatsoDev/LinkedSouls.git
   cd LinkedSouls
   ```

2. **Initialize Git LFS** (for large binary files)
   ```bash
   git lfs install
   git lfs pull
   ```

3. **Generate project files**
   - Right-click `LinkedSouls.uproject` → **Generate Visual Studio project files**
   - Or run: `"C:\Program Files\Epic Games\UE_5.x\Engine\Build\BatchFiles\GenerateProjectFiles.bat" LinkedSouls.uproject`

4. **Open in Visual Studio**
   - Open the generated `.sln` file
   - Set configuration to `Development Editor`
   - Build the project (Ctrl+Shift+B)

5. **Launch in Unreal Editor**
   - Open `LinkedSouls.uproject` directly or via Visual Studio
   - Let shaders compile on first launch

### First Time Setup

1. Open the project in Unreal Editor
2. Wait for shader compilation to complete
3. Navigate to `Content/Maps/` to find the main level
4. Press **Play** to test in single-player mode

## Testing Multiplayer (Listen Server)

### Local Testing Method

1. **Package the Game** (Recommended)
   - Go to **File → Package Project → Windows → Windows (64-bit)**
   - Choose an output directory
   - Wait for packaging to complete

2. **Run Two Instances**
   - Launch the packaged game executable twice
   - In the first instance, click **Host Game** (starts as Listen Server)
   - In the second instance, connect to `localhost` or use direct IP

### Editor Testing

1. **Configure Multiplayer Settings**
   - Open **Edit → Project Settings → Maps & Modes**
   - Set **Number of Players** to `2`
   - Set **Net Mode** to `Play As Listen Server`

2. **Play in Editor**
   - Click **Play** → Select **Standalone** or **PIE (Play In Editor)**
   - Use `open 127.0.0.1` in console for second player

### Dedicated Server Testing

```bash
# Start server
LinkedSoulsServer.exe -log

# Start clients
LinkedSoulsClient.exe 127.0.0.1 -game -log
```

## Development Workflow

### Building Changes

1. Make C++ changes in `Source/LinkedSouls/`
2. Save files → Hot Reload or Rebuild in Editor
3. Test changes immediately in Play mode

### Blueprints

- Modify Blueprints in `Content/Blueprints/`
- Create new abilities in `Content/Abilities/`
- Test in **Play In Editor (PIE)** mode

### Debugging

- Use **Output Log** for console messages
- Enable **Verbose Logging** in Project Settings
- Use **Network Profiler** for multiplayer debugging

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please read our contributing guidelines before submitting pull requests.

---

**Built with Unreal Engine 5 | C++ | Enhanced Input System**