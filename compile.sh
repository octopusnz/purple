#=========================================================================
#    Purple
#    https://github.com/octopusnz/purple
#    Copyright (c) 2026 Jacob Doherty
#    SPDX-License-Identifier: MIT
#    File: compile.sh
#    Description: Bash build script to assist with compiling
#=========================================================================


#!/usr/bin/env bash
set -euo pipefail
set -E
# Ensure predictable field splitting
IFS=$' \t\n'

# Enable alias expansion in non-interactive shell
shopt -s expand_aliases
alias gcc='/usr/local/bin/gcc-15.2.0'
alias clang='/usr/local/bin/clang'
alias clang-tidy='/usr/local/bin/clang-tidy'
alias scan-build='/usr/local/bin/scan-build'

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
echo "Compiling with GCC...:"

# Function to format elapsed time (shows ms if < 1s, otherwise seconds)
format_elapsed_time() {
    local elapsed_ms=$1
    if [ "$elapsed_ms" -lt 1000 ]; then
        echo "${elapsed_ms}ms"
    else
        local elapsed_s=$((elapsed_ms / 1000))
        echo "${elapsed_s}s"
    fi
}

# Set compilation flags based on mode
BUILD_START_TIME=$(date +%s%3N)
if [ "$DEBUG_MODE" = true ]; then
    # Debug build with sanitizers
    GCC_LOG="logs/gcc_$(date +%Y-%m-%d_%H-%M-%S).log"
    {
        echo "=== GCC Compilation ==="
        echo "GCC Version: $(gcc --version | head -1)"
        echo "Started: $(date)"
        echo ""
        echo "--- Building main-asan ---"
        gcc main.c ball.c paddle.c resource.c leaderboard.c -o build/main-asan \
            -Wall -Wextra -Wpedantic -Wunused -Wshadow -Wconversion \
            -Wsign-conversion -Wdouble-promotion -Wformat=2 \
            -fno-omit-frame-pointer -fanalyzer -std=c99 -fsanitize=address \
            -lraylib -lm -lpthread -ldl -lrt -lX11 2>&1
        echo ""
        echo "--- Building main-ubsan ---"
        gcc main.c ball.c paddle.c resource.c leaderboard.c -o build/main-ubsan \
            -Wall -Wextra -Wpedantic -Wunused -Wshadow -Wconversion \
            -Wsign-conversion -Wdouble-promotion -Wformat=2 \
            -fno-omit-frame-pointer -fanalyzer -std=c99 \
            -fsanitize=undefined -fno-sanitize-recover=undefined \
            -lraylib -lm -lpthread -ldl -lrt -lX11 2>&1
        echo ""
        echo "--- Building main-valgrind ---"
        gcc main.c ball.c paddle.c resource.c leaderboard.c -o build/main-valgrind \
            -Wall -Wextra -Wpedantic -Wunused -Wshadow -Wconversion \
            -Wsign-conversion -Wdouble-promotion -Wformat=2 \
            -fno-omit-frame-pointer -fanalyzer -std=c99 \
            -lraylib -lm -lpthread -ldl -lrt -lX11 2>&1
        echo ""
        echo "Completed: $(date)"
    } > "$GCC_LOG" 2>&1
    
    # Clean up old GCC log files (keep only 2 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/gcc_*.log 2>/dev/null | wc -l)" -gt 2 ]; then
        # shellcheck disable=SC2012
        ls -t logs/gcc_*.log | sed -n '3,$p' | while read -r file; do
            rm "$file"
        done
    fi
    
    echo "Compiling with Clang..."
    CLANG_LOG="logs/clang_$(date +%Y-%m-%d_%H-%M-%S).log"
    {
        echo "=== Clang Compilation ==="
        echo "Clang Version: $(clang --version | head -1)"
        echo "Started: $(date)"
        echo ""
        echo "--- Building main-clang ---"
        clang main.c ball.c paddle.c resource.c leaderboard.c -o build/main-clang \
            -Wall -Wextra -Wpedantic -Wunused -Wshadow -Wconversion \
            -Wsign-conversion -Wdouble-promotion -Wformat=2 -std=c99 \
            -lraylib -lm -lpthread -ldl -lrt -lX11 2>&1
        echo ""
        echo "Completed: $(date)"
    } > "$CLANG_LOG" 2>&1
    
    # Clean up old Clang log files (keep only 2 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/clang_*.log 2>/dev/null | wc -l)" -gt 2 ]; then
        # shellcheck disable=SC2012
        ls -t logs/clang_*.log | sed -n '3,$p' | while read -r file; do
            rm "$file"
        done
    fi
elif [ "$TEST_MODE" = true ]; then
    # Test build
    echo "Compiling tests..."
    gcc ball.c paddle.c resource.c leaderboard.c \
        /usr/local/include/unity/unity.c test/test.c \
        -o build/test_runner -Wall -Wextra -Wpedantic -std=c99 -I. \
        -lraylib -lm -lpthread -ldl -lrt -lX11
else
    # Production build with size optimizations
    gcc main.c ball.c paddle.c resource.c leaderboard.c -o build/main \
        -Wall -Wextra -Wpedantic -std=c99 -Os -s -flto \
        -ffunction-sections -fdata-sections -fomit-frame-pointer \
        -fno-asynchronous-unwind-tables -fno-unwind-tables \
        -Wl,--gc-sections -Wl,--as-needed -Wl,-O1 \
        -lraylib -lm -lpthread -ldl -lrt -lX11
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
        echo "Cppcheck Version: $(cppcheck --version)"
        echo "Started: $(date)"
        echo ""
        cppcheck --check-level=exhaustive --enable=all --inconclusive \
            --verbose --force --suppress=missingIncludeSystem --std=c99 \
            --checkers-report="$CHECKERS_REPORT" main.c 2>&1 || true
        echo ""
        echo "=== Checkers Report ==="
        cat "$CHECKERS_REPORT"
        echo ""
        echo "Completed: $(date)"
    } > "$CPPCHECK_LOG"
    rm "$CHECKERS_REPORT"
    
    # Clean up old cppcheck log files (keep only 2 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/cppcheck_*.log 2>/dev/null | wc -l)" -gt 2 ]; then
        # shellcheck disable=SC2012
        ls -t logs/cppcheck_*.log | sed -n '3,$p' | while read -r file; do
            rm "$file"
        done
    fi
    
    echo "Running clang-tidy static analysis..."
    CLANGTIDY_LOG="logs/clang-tidy_$(date +%Y-%m-%d_%H-%M-%S).log"
    {
        echo "=== Clang-Tidy Static Analysis ==="
        echo "Clang-Tidy Version: $(clang-tidy --version 2>&1 | grep -i 'version' | head -1)"
        echo "Started: $(date)"
        echo ""
        clang-tidy main.c ball.c paddle.c resource.c leaderboard.c -- \
            -std=c99 -I. -I/usr/include 2>&1 || true
        echo ""
        echo "Completed: $(date)"
    } > "$CLANGTIDY_LOG"
    
    # Clean up old clang-tidy log files (keep only 2 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/clang-tidy_*.log 2>/dev/null | wc -l)" -gt 2 ]; then
        # shellcheck disable=SC2012
        ls -t logs/clang-tidy_*.log | sed -n '3,$p' | while read -r file; do
            rm "$file"
        done
    fi
    
    echo "Running scan-build static analysis..."
    SCANBUILD_LOG="logs/scan-build_$(date +%Y-%m-%d_%H-%M-%S).log"
    {
        echo "=== Scan-Build Static Analysis ==="
        echo "Clang Version: $(clang --version 2>&1 | grep -i 'version' | head -1)"
        echo "Started: $(date)"
        echo ""
        scan-build -o build/scan-build-results gcc main.c ball.c paddle.c \
            resource.c leaderboard.c -o /dev/null -std=c99 \
            -lraylib -lm -lpthread -ldl -lrt -lX11 2>&1 || true
        echo ""
        echo "Completed: $(date)"
    } > "$SCANBUILD_LOG"
    
    # Clean up old scan-build log files (keep only 2 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/scan-build_*.log 2>/dev/null | wc -l)" -gt 2 ]; then
        # shellcheck disable=SC2012
        ls -t logs/scan-build_*.log | sed -n '3,$p' | while read -r file; do
            rm "$file"
        done
    fi
    
    echo "Running AddressSanitizer check..."
    ASAN_LOG="logs/asan_$(date +%Y-%m-%d_%H-%M-%S).log"
    {
        echo "=== AddressSanitizer Analysis ==="
        echo "GCC Version: $(gcc --version | head -1)"
        echo "Sanitizer: AddressSanitizer (ASAN)"
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
    
    # Clean up old ASAN log files (keep only 2 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/asan_*.log 2>/dev/null | wc -l)" -gt 2 ]; then
        # shellcheck disable=SC2012
        ls -t logs/asan_*.log | sed -n '3,$p' | while read -r file; do
            rm "$file"
        done
    fi

    echo "Running UBSan check..."
    UBSAN_LOG="logs/ubsan_$(date +%Y-%m-%d_%H-%M-%S).log"
    {
        echo "=== UndefinedBehaviorSanitizer Analysis ==="
        echo "GCC Version: $(gcc --version | head -1)"
        echo "Sanitizer: UndefinedBehaviorSanitizer (UBSan)"
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

    # Clean up old UBSan log files (keep only 2 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/ubsan_*.log 2>/dev/null | wc -l)" -gt 2 ]; then
        # shellcheck disable=SC2012
        ls -t logs/ubsan_*.log | sed -n '3,$p' | while read -r file; do
            rm "$file"
        done
    fi
    
    echo "Checking with Valgrind for memory issues..."
    VALGRIND_LOG="logs/valgrind_$(date +%Y-%m-%d_%H-%M-%S).log"
    {
        echo "=== Valgrind Memory Analysis ==="
        echo "Valgrind Version: $(valgrind --version)"
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
    
    
    # Clean up old Valgrind log files (keep only 2 most recent)
    # shellcheck disable=SC2012
    if [ "$(ls logs/valgrind_*.log 2>/dev/null | wc -l)" -gt 2 ]; then
        # shellcheck disable=SC2012
        ls -t logs/valgrind_*.log | sed -n '3,$p' | while read -r file; do
            rm "$file"
        done
    fi
    TOTAL_END_TIME=$(date +%s%3N)
    TOTAL_ELAPSED_MS=$((TOTAL_END_TIME - BUILD_START_TIME))
    TOTAL_ELAPSED=$(format_elapsed_time "$TOTAL_ELAPSED_MS")
    echo ""
    echo "Total debug execution time: ${TOTAL_ELAPSED}"
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
    TOTAL_END_TIME=$(date +%s%3N)
    TOTAL_ELAPSED_MS=$((TOTAL_END_TIME - BUILD_START_TIME))
    TOTAL_ELAPSED=$(format_elapsed_time "$TOTAL_ELAPSED_MS")
    echo ""
    echo "Total test execution time: ${TOTAL_ELAPSED}"
elif [ "$DEBUG_MODE" = false ] && [ "$TEST_MODE" = false ]; then
    # Production mode total time
    TOTAL_END_TIME=$(date +%s%3N)
    TOTAL_ELAPSED_MS=$((TOTAL_END_TIME - BUILD_START_TIME))
    TOTAL_ELAPSED=$(format_elapsed_time "$TOTAL_ELAPSED_MS")
    echo ""
    echo "Total build time: ${TOTAL_ELAPSED}"
fi


