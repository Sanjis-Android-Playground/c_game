# Demon FPS 🔥

A first-person shooter Android game built with **raylib** and **raymob**. Fight demons in a raycasted 3D maze!

![Demon FPS](app/src/main/ic_launcher-playstore.png)

## Features 🎮

- **Raycasting 3D Engine** - Classic Wolfenstein-style rendering
- **Touch Controls** - Virtual joystick + drag-to-look camera
- **Combat System** - Shoot demons, reload, manage ammo
- **Multiple Enemy Types** - Imp, Demon, Caco, Baron (each with different colors)
- **Particle Effects** - Muzzle flash, blood splatter, death explosions
- **Pause Menu** - Adjustable sensitivity and volume sliders
- **Main Menu** - Play, Settings, Exit

## Controls 📱

| Control | Action |
|---------|--------|
| **Left Joystick** | Move (WASD style) |
| **Drag Right Side** | Look around |
| **FIRE Button** | Shoot |
| **R Button** | Reload |
| **II Button** | Pause |

## Building from Source 🛠️

### Prerequisites

- Android SDK API 34
- Android NDK r26+ (compatible with CMake 3.30.3)
- Java 17

### Setup

1. **Clone the repository with submodules:**
```bash
git clone --recurse-submodules https://github.com/Sanjis-Android-Playground/c_game.git
cd c_game
```

2. **Set environment variables:**
```bash
export ANDROID_HOME=/path/to/android-sdk
export JAVA_HOME=/path/to/java-17
```

3. **Build the APK:**
```bash
./gradlew assembleDebug
```

The APK will be at:
```
app/build/outputs/apk/debug/app-debug.apk
```

### Build for Release

```bash
./gradlew assembleRelease
```

## Project Structure 📁

```
c_game/
├── app/src/main/cpp/
│   ├── main.c           # Game code (raycasting, AI, controls)
│   ├── CMakeLists.txt   # Build config
│   └── deps/
│       ├── raylib/      # raylib submodule
│       └── raymob/      # raymob submodule
├── app/src/main/java/   # Java wrapper
├── app/build.gradle     # App build config
├── gradle.properties    # App name, version, etc.
└── README.md           # This file
```

## Game Mechanics 🎯

### Player
- **Health:** 100 HP
- **Ammo:** 30 rounds (reload with R button)
- **Score:** Kill demons to increase score

### Enemies
| Type | Color | Speed |
|------|-------|-------|
| Imp | Red | Slow |
| Demon | Orange | Medium |
| Caco | Purple | Fast |
| Baron | Maroon | Very Fast |

### Map
16x16 tile-based maze with walls. Demons spawn randomly and chase the player.

## Customization 🎨

Edit `app/src/main/cpp/main.c` to modify:
- `MAP_SIZE` - Maze size
- `MOVE_SPEED` - Player speed
- `MAX_DEMONS` - Max enemies
- `map[][]` - Level layout (1=wall, 0=empty)

## Credits 🙏

- Built with [raylib](https://raylib.com) - Simple and easy-to-use game library
- Using [raymob](https://github.com/Bigfoot71/raymob) - raylib for Android

## License 📄

MIT License - Feel free to use, modify, and distribute!

---

**Made with ❤️ by AI (Esdeath) for Sanji**
