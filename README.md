# Purple

A classic Pong game implementation in C using Raylib, featuring AI opponent, dynamic difficulty, and persistent leaderboards.

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

Builds multiple variants with sanitizers and runs static analysis.

### Test Build

```bash
./compile.sh --test
```

Compiles and runs the unit test suite using Unity framework.

### Fuzz Testing

```bash
./compile.sh --fuzz       # Quick run: 60s per target (5 min total)
./compile.sh --fuzz-long  # Extended run: 12 min per target (60 min total)
```

Runs coverage-guided fuzz testing on all game components using libFuzzer with AddressSanitizer and UndefinedBehaviorSanitizer.

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
├── main.c                   # Game loop, state management, and rendering
├── ball.c/h                 # Ball physics and collision detection
├── paddle.c/h               # Paddle movement and AI logic
├── leaderboard.c/h          # Leaderboard persistence and sorting
├── resource.c/h             # Resource file discovery with fallback paths
├── compile.sh               # Build script with multiple modes (production/debug/test/fuzz)
├── .github/
│   └── copilot-instructions.md  # GitHub Copilot configuration
├── fuzz/
│   ├── corpus/              # Persistent fuzzing corpus (test cases)
│   ├── fuzz_ball_collision.c      # Ball/paddle collision fuzzer
│   ├── fuzz_paddle_position.c     # Paddle boundary fuzzer
│   ├── fuzz_leaderboard.c         # Leaderboard sorting fuzzer
│   ├── fuzz_ai_paddle.c           # AI decision making fuzzer
│   └── fuzz_game_physics.c        # Multi-frame gameplay fuzzer
├── test/
│   └── test.c               # Unit tests (81 tests using Unity framework)
├── resources/
│   ├── orbitron/            # Orbitron variable font files
│   ├── RAY-LICENSE.txt      # Raylib license
│   ├── UNITY-LICENSE.txt    # Unity test framework license
│   └── OFL-LICENSE.txt      # Orbitron font license
├── build/                   # Compiled binaries and artifacts
│   ├── main                 # Production binary
│   ├── test_runner          # Test suite binary
│   ├── fuzz_*               # Fuzzing binaries (5 targets)
│   └── fuzz_artifacts/      # Crash and leak reports from fuzzing
└── logs/                    # Debug analysis logs (ASAN, UBSan, Valgrind, etc.)
```

## Testing

### Unit Tests

The project includes 81 comprehensive unit tests covering:

- Ball physics and collision detection
- Paddle movement and boundary handling
- AI paddle logic and edge cases
- Leaderboard sorting and persistence
- Resource file discovery
- NaN/Inf handling and sanitization

Run tests with:

```bash
./compile.sh --test
```

### Fuzz Test

Five libFuzzer targets provide coverage-guided testing:

1. **fuzz_ball_collision**: Tests ball/paddle collisions, spin mechanics, and pushback
2. **fuzz_paddle_position**: Tests paddle boundary clamping with various sizes
3. **fuzz_leaderboard**: Tests entry sorting, max capacity, and edge cases
4. **fuzz_ai_paddle**: Tests AI decision making and movement
5. **fuzz_game_physics**: Tests realistic multi-frame gameplay scenarios

All fuzzers use AddressSanitizer and UndefinedBehaviorSanitizer for memory safety validation.

### Debug Analysis

Debug mode runs multiple static and dynamic analysis tools:

- **GCC/Clang compilation** with all warnings enabled
- **cppcheck**: Exhaustive static analysis
- **clang-tidy**: Lint checks and best practices
- **scan-build**: Clang static analyzer
- **AddressSanitizer (ASAN)**: Memory error detection
- **UndefinedBehaviorSanitizer (UBSan)**: Undefined behavior detection
- **Valgrind**: Memory leak detection

All logs are saved to `logs/` directory with timestamps.

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
