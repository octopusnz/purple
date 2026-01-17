#!/usr/bin/env bash
set -euo pipefail
set -E
# Ensure predictable field splitting
IFS=$' \t\n'

# Enable alias expansion in non-interactive shell
shopt -s expand_aliases
alias gcc='/usr/local/bin/gcc-15.2.0'

# Check for mode
DEBUG_MODE=false
TEST_MODE=false
if [ $# -gt 0 ]; then
    if [ "$1" = "--debug" ] || [ "$1" = "debug" ]; then
        DEBUG_MODE=true
        echo "Building in DEBUG mode with ASAN, UBSan, and Valgrind checks..."
    elif [ "$1" = "--test" ] || [ "$1" = "test" ]; then
        TEST_MODE=true
        echo "Building and running UNIT TESTS..."
    elif [ "$1" = "--clean" ] || [ "$1" = "clean" ]; then
        echo "Cleaning binaries and object files..."
        if [ -d build ]; then
            echo "Removing build directory"
            rm -rf build
        fi
        echo "Clean complete."
        exit 0
    else
        echo "Error: Invalid argument '$1'"
        echo "Usage: $0 [--debug|debug|--test|test|--clean|clean]"
        echo "  No arguments: Production build with optimizations"
        echo "  --debug or debug: Debug build with ASAN, UBSan, and Valgrind checks"
        echo "  --test or test: Build and run unit tests"
        echo "  --clean or clean: Remove all binaries and object files"
        exit 1
    fi
else
    echo "Building in PRODUCTION mode with optimizations..."
fi

# Create logs directory if it doesn't exist (only needed for debug mode)
if [ "$DEBUG_MODE" = true ]; then
    mkdir -p logs
fi

mkdir -p build

if [ -f build/main ]; then
    echo "Removing old production binary..."
    rm build/main
fi

if [ -f build/main-asan ]; then
    echo "Removing old ASAN binary..."
    rm build/main-asan
fi

if [ -f build/main-valgrind ]; then
    echo "Removing old Valgrind binary..."
    rm build/main-valgrind
fi
if [ -f build/main-ubsan ]; then
    echo "Removing old UBSan binary..."
    rm build/main-ubsan
fi
if [ -f main-ubsan ]; then
    echo "Removing old UBSan binary..."
    rm main-ubsan
fi
echo "Compiling with GCC version:"
gcc --version

# Set compilation flags based on mode
if [ "$DEBUG_MODE" = true ]; then
    # Debug build with sanitizers
    GCC_LOG="logs/gcc_$(date +%Y-%m-%d_%H-%M-%S).log"
    echo "Capturing compiler output to $GCC_LOG..."
    {
        echo "=== GCC Compilation ==="
        echo "Started: $(date)"
        echo ""
        echo "--- Building main-asan ---"
        gcc main.c ball.c paddle.c -o build/main-asan -Wall -Wextra -Wpedantic -Wunused -Wshadow -Wconversion -Wsign-conversion -Wdouble-promotion -Wformat=2 -fno-omit-frame-pointer -fanalyzer -std=c99 -fsanitize=address -lraylib -lm -lpthread -ldl -lrt -lX11 2>&1
        echo ""
        echo "--- Building main-ubsan ---"
        gcc main.c ball.c paddle.c -o build/main-ubsan -Wall -Wextra -Wpedantic -Wunused -Wshadow -Wconversion -Wsign-conversion -Wdouble-promotion -Wformat=2 -fno-omit-frame-pointer -fanalyzer -std=c99 -fsanitize=undefined -fno-sanitize-recover=undefined -lraylib -lm -lpthread -ldl -lrt -lX11 2>&1
        echo ""
        echo "--- Building main-valgrind ---"
        gcc main.c ball.c paddle.c -o build/main-valgrind -Wall -Wextra -Wpedantic -Wunused -Wshadow -Wconversion -Wsign-conversion -Wdouble-promotion -Wformat=2 -fno-omit-frame-pointer -fanalyzer -std=c99 -lraylib -lm -lpthread -ldl -lrt -lX11 2>&1
        echo ""
        echo "Completed: $(date)"
    } > "$GCC_LOG" 2>&1
    
    # Clean up old GCC log files (keep only 3 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/gcc_*.log 2>/dev/null | wc -l)" -gt 3 ]; then
        echo "Cleaning up old GCC logs..."
        # shellcheck disable=SC2012
        ls -t logs/gcc_*.log | sed -n '4,$p' | while read -r file; do
            echo "Removing $file"
            rm "$file"
        done
    fi
elif [ "$TEST_MODE" = true ]; then
    # Test build
    echo "Compiling tests..."
    gcc ball.c paddle.c /usr/local/include/unity/unity.c test/test.c -o build/test_runner -Wall -Wextra -Wpedantic -std=c99 -I. -lraylib -lm -lpthread -ldl -lrt -lX11
else
    # Production build with size optimizations
    gcc main.c ball.c paddle.c -o build/main -Wall -Wextra -Wpedantic -std=c99 -Os -s -flto -lraylib -lm -lpthread -ldl -lrt -lX11
    strip --strip-all build/main
    echo "Production build complete"
fi

# Run debug checks only in debug mode
if [ "$DEBUG_MODE" = true ]; then
    echo "Running cppcheck static analysis..."
    CPPCHECK_LOG="logs/cppcheck_$(date +%Y-%m-%d_%H-%M-%S).log"
    CHECKERS_REPORT=$(mktemp)
    {
        echo "=== Cppcheck Static Analysis ==="
        echo "Started: $(date)"
        echo ""
        cppcheck --enable=all --inconclusive --verbose --force --suppress=missingIncludeSystem --std=c99 --checkers-report="$CHECKERS_REPORT" main.c 2>&1 || true
        echo ""
        echo "=== Checkers Report ==="
        cat "$CHECKERS_REPORT"
        echo ""
        echo "Completed: $(date)"
    } > "$CPPCHECK_LOG"
    rm "$CHECKERS_REPORT"
    
    # Clean up old cppcheck log files (keep only 3 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/cppcheck_*.log 2>/dev/null | wc -l)" -gt 3 ]; then
        echo "Cleaning up old cppcheck logs..."
        # shellcheck disable=SC2012
        ls -t logs/cppcheck_*.log | sed -n '4,$p' | while read -r file; do
            echo "Removing $file"
            rm "$file"
        done
    fi
    
    echo "Running clang-tidy static analysis..."
    CLANGTIDY_LOG="logs/clang-tidy_$(date +%Y-%m-%d_%H-%M-%S).log"
    {
        echo "=== Clang-Tidy Static Analysis ==="
        echo "Started: $(date)"
        echo ""
        clang-tidy main.c -- -std=c99 -I/usr/include -lraylib -lm -lpthread -ldl -lrt -lX11 2>&1 || true
        echo ""
        echo "Completed: $(date)"
    } > "$CLANGTIDY_LOG"
    
    # Clean up old clang-tidy log files (keep only 3 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/clang-tidy_*.log 2>/dev/null | wc -l)" -gt 3 ]; then
        echo "Cleaning up old clang-tidy logs..."
        # shellcheck disable=SC2012
        ls -t logs/clang-tidy_*.log | sed -n '4,$p' | while read -r file; do
            echo "Removing $file"
            rm "$file"
        done
    fi
    
    echo "Running AddressSanitizer check..."
    ASAN_LOG="logs/asan_$(date +%Y-%m-%d_%H-%M-%S).log"
    {
        echo "=== AddressSanitizer Analysis ==="
        echo "Started: $(date)"
        echo ""
        timeout 5 ./build/main-asan 2>&1 || true
        echo ""
        echo "=== Analysis Complete ==="
    } > "$ASAN_LOG" 2>&1
    
    # Add status summary
    if grep -q "ERROR: AddressSanitizer" "$ASAN_LOG"; then
        echo "Status: MEMORY ERRORS DETECTED" >> "$ASAN_LOG"
    else
        echo "Status: No memory errors detected" >> "$ASAN_LOG"
    fi
    echo "Completed: $(date)" >> "$ASAN_LOG"
    
    # Clean up old ASAN log files (keep only 3 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/asan_*.log 2>/dev/null | wc -l)" -gt 3 ]; then
        echo "Cleaning up old ASAN logs..."
        # shellcheck disable=SC2012
        ls -t logs/asan_*.log | sed -n '4,$p' | while read -r file; do
            echo "Removing $file"
            rm "$file"
        done
    fi

    echo "Running UBSan check..."
    UBSAN_LOG="logs/ubsan_$(date +%Y-%m-%d_%H-%M-%S).log"
    {
        echo "=== UndefinedBehaviorSanitizer Analysis ==="
        echo "Started: $(date)"
        echo ""
        timeout 5 ./build/main-ubsan 2>&1 || true
        echo ""
        echo "=== Analysis Complete ==="
    } > "$UBSAN_LOG" 2>&1

    # Add status summary
    if grep -q "runtime error:" "$UBSAN_LOG"; then
        echo "Status: UNDEFINED BEHAVIOR DETECTED" >> "$UBSAN_LOG"
    else
        echo "Status: No undefined behavior detected" >> "$UBSAN_LOG"
    fi
    echo "Completed: $(date)" >> "$UBSAN_LOG"

    # Clean up old UBSan log files (keep only 3 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/ubsan_*.log 2>/dev/null | wc -l)" -gt 3 ]; then
        echo "Cleaning up old UBSan logs..."
        # shellcheck disable=SC2012
        ls -t logs/ubsan_*.log | sed -n '4,$p' | while read -r file; do
            echo "Removing $file"
            rm "$file"
        done
    fi
    
    echo "Checking with Valgrind for memory issues..."
    VALGRIND_LOG="logs/valgrind_$(date +%Y-%m-%d_%H-%M-%S).log"
    {
        echo "=== Valgrind Memory Analysis ==="
        echo "Started: $(date)"
        echo ""
        timeout 5 valgrind --leak-check=full -s ./build/main-valgrind 2>&1 || true
        echo ""
        echo "=== Analysis Complete ==="
    } > "$VALGRIND_LOG" 2>&1
    
    # Add status summary
    if grep -q "ERROR SUMMARY: [1-9]" "$VALGRIND_LOG"; then
        echo "Status: MEMORY ERRORS DETECTED" >> "$VALGRIND_LOG"
    else
        echo "Status: No critical memory errors detected" >> "$VALGRIND_LOG"
    fi
    echo "Completed: $(date)" >> "$VALGRIND_LOG"
    
    
    # Clean up old Valgrind log files (keep only 3 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/valgrind_*.log 2>/dev/null | wc -l)" -gt 3 ]; then
        echo "Cleaning up old Valgrind logs..."
        # shellcheck disable=SC2012
        ls -t logs/valgrind_*.log | sed -n '4,$p' | while read -r file; do
            echo "Removing $file"
            rm "$file"
        done
    fi
fi

# Run tests if in test mode
if [ "$TEST_MODE" = true ]; then
    echo ""
    echo "Running unit tests..."
    echo "===================="
    ./build/test_runner
    TEST_EXIT_CODE=$?
    echo "===================="
    if [ $TEST_EXIT_CODE -eq 0 ]; then
        echo "✓ All tests passed!"
    else
        echo "✗ Tests failed with exit code $TEST_EXIT_CODE"
        exit $TEST_EXIT_CODE
    fi
fi


