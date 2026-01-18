# Purple

A classic Pong game implementation in C using Raylib, featuring AI opponent, dynamic difficulty, and persistent leaderboards.

## Features

- **Classic Pong Gameplay**: Play against an AI opponent with realistic physics
- **Dynamic Difficulty**: Ball speed increases with total points scored
- **Persistent Leaderboard**: Tracks the fastest wins (time to reach 5 points)
- **Spin Mechanics**: Ball trajectory affected by paddle hit position
- **Responsive AI**: Imperfect AI opponent for challenging but fair gameplay
- **Custom Font**: Uses Orbitron variable font for clean, modern UI

## Requirements

- GCC 15.2.0 or Clang (for compilation)
- Raylib library
- Unity test framework (for running tests)
- Linux environment (tested on Linux)

## Building

### Production Build

```bash
./compile.sh
```

Produces an optimized binary at `build/main` with size optimizations and stripped symbols.

### Debug Build

```bash
./compile.sh --debug
```

Builds multiple variants with sanitizers and runs static analysis:

- AddressSanitizer (ASAN) build
- UndefinedBehaviorSanitizer (UBSan) build
- Valgrind-compatible build
- Clang compilation
- Static analysis (cppcheck, clang-tidy, scan-build)

All debug logs are saved to the `logs/` directory.

### Test Build

```bash
./compile.sh --test
```

Compiles and runs the unit test suite using Unity framework.

### Clean Build

```bash
./compile.sh --clean
```

Removes all build artifacts.

## Running

After building, run the game:

```bash
./build/main
```

## Controls

- **Arrow Up/Down**: Move player paddle
- **Space**: Start game (from start screen)
- **Enter**: Submit initials (after winning)
- **Backspace**: Delete initials characters
- **A-Z**: Enter initials (automatically capitalized)

## Gameplay

- First player to **5 points** wins
- Ball speed increases gradually as total points accumulate
- Hitting the ball near paddle edges adds vertical spin
- AI automatically records wins; players enter initials
- Leaderboard shows top 10 fastest wins sorted by completion time

## Project Structure

```text
purple/
├── main.c              # Game loop and rendering
├── ball.c/h            # Ball physics and collision detection
├── paddle.c/h          # Paddle movement and AI logic
├── leaderboard.c/h     # Leaderboard persistence
├── resource.c/h        # Resource file discovery
├── test/
│   └── test.c          # Unit tests (Unity framework)
├── resources/
│   └── orbitron/       # Orbitron font files
├── build/              # Compiled binaries
└── logs/               # Debug analysis logs
```

## Leaderboard

The leaderboard is stored at `$HOME/.purple/leaderboard.txt` and tracks:

- Player initials (3 characters)
- Winner type (P = Player, A = AI)
- Time to win (in seconds)

Only the 10 fastest wins are kept, sorted by completion time.

## Credits

- **Graphics Library**: [Raylib](https://www.raylib.com/)
- **Test Framework**: [Unity](https://www.throwtheswitch.org/unity)
- **Font**: [Orbitron](https://fonts.google.com/specimen/Orbitron)

## License

MIT License - see [LICENSE.txt](LICENSE.txt) for details.

Third-party licenses:

- Raylib: [resources/RAY-LICENSE.txt](resources/RAY-LICENSE.txt)
- Unity Test Framework: [resources/UNITY-LICENSE.txt](resources/UNITY-LICENSE.txt)
- Orbitron Font: [resources/orbitron/OFL-LICENSE.txt](resources/orbitron/OFL-LICENSE.txt)
