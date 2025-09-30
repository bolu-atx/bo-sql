#!/bin/bash

set -e

BINARY="${BINARY:-}"
if [ -z "$BINARY" ]; then
  if [ -f "./build-dev/bq" ]; then
    BINARY="./build-dev/bq"
  elif [ -f "./build/bq" ]; then
    BINARY="./build/bq"
  elif [ -n "$MESON_BUILD_ROOT" ] && [ -f "$MESON_BUILD_ROOT/bq" ]; then
    BINARY="$MESON_BUILD_ROOT/bq"
  fi
fi

BINARY=${BINARY:-./build/bq}

TEST_CSV="test.csv"

if [ ! -f "$BINARY" ]; then
  echo "Error: Binary $BINARY not found. Run 'meson compile -C build' first."
  exit 1
fi

if [ ! -f "$TEST_CSV" ]; then
  echo "Error: Test CSV $TEST_CSV not found."
  exit 1
fi

echo "# Smoke Test Report"
echo "Generated on $(date)"
echo ""

run_test() {
  local description="$1"
  local command="$2"
  local expected="$3"

  echo "## Test: $description"
  echo "### Command"
  echo "\`\`\`bash"
  echo "$command"
  echo "\`\`\`"
  echo "### Expected"
  echo "\`\`\`"
  echo "$expected"
  echo "\`\`\`"

  # Run command and capture output
  local actual
  if ! actual=$(eval "$command" 2>&1); then
    actual="COMMAND FAILED: $actual"
  fi

  echo "### Actual"
  echo "\`\`\`"
  echo "$actual"
  echo "\`\`\`"

  # Check status
  if echo "$actual" | grep -q "$expected"; then
    echo "### Status"
    echo "✅ PASS"
  else
    echo "### Status"
    echo "❌ FAIL"
  fi
  echo ""
}

# Test 1: Binary runs without args (REPL startup)
run_test "Binary runs and shows REPL prompt" "timeout 2 $BINARY <<< 'EXIT'" "bq CLI"

# Test 2: Load CSV and enter REPL
run_test "Load CSV and enter REPL" "timeout 2 $BINARY $TEST_CSV <<< 'EXIT'" "Loaded table"

# Test 3: SQL argument with CSV file
run_test "Execute SQL argument with CSV file" "$BINARY $TEST_CSV --sql 'SELECT * FROM table LIMIT 1'" "|"

# Test 4: SQL argument with CSV output format
run_test "Execute SQL with CSV formatter" "$BINARY $TEST_CSV --sql 'SELECT * FROM table LIMIT 1' --output-format csv" "id,name,value"

# Test 5: SQL argument with CSV from stdin
run_test "Execute SQL on CSV from stdin" "printf 'id,name\\n1,Alice\\n2,Bob\\n' | $BINARY --sql 'SELECT * FROM table'" "| id | name  |"

# Test 6: Set output format in REPL
run_test "REPL SET FORMAT command" "printf 'SET FORMAT csv\\nSELECT * FROM table LIMIT 1\\nEXIT\\n' | timeout 2 $BINARY $TEST_CSV" "Output format set to csv"

# Test 7: Invalid argument
run_test "Invalid argument error" "$BINARY --invalid" "Unknown option"

echo "Smoke tests completed."
