#!/bin/bash

set -e

BINARY="./build-dev/bq"
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

# Test 3: SQL from stdin with CSV file
run_test "Execute SQL from stdin with CSV file" "echo 'SELECT * FROM table LIMIT 1' | $BINARY $TEST_CSV --sql" "|"

# Test 4: SQL from stdin with CSV output format
run_test "Execute SQL with CSV formatter" "echo 'SELECT * FROM table LIMIT 1' | $BINARY $TEST_CSV --sql --output-format csv" "id,name,value"

# Test 5: SQL and CSV from stdin
run_test "Execute SQL on CSV from stdin" "echo -e 'SELECT * FROM table\nid,name\n1,Alice\n2,Bob\n' | $BINARY --sql" "| id | name  |"

# Test 6: Set output format in REPL
run_test "REPL SET FORMAT command" "printf 'SET FORMAT csv\\nSELECT * FROM table LIMIT 1\\nEXIT\\n' | timeout 2 $BINARY $TEST_CSV" "Output format set to csv"

# Test 7: Invalid argument
run_test "Invalid argument error" "$BINARY --invalid" "Unknown option"

echo "Smoke tests completed."
