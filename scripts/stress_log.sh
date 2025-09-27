#!/usr/bin/env bash
set -euo pipefail

# Configuration
build_dir=build
threads=8
lines=50000

echo "Building project..."
make clean
make -j

echo "Cleaning logs directory..."
rm -rf logs && mkdir -p logs

echo "Running stress test with $threads threads, $lines lines each..."
./build/logtest --threads $threads --lines $lines --out logs/test.log

expected=$((threads*lines))
actual=$(wc -l < logs/test.log | tr -d ' ')

echo "Esperado: $expected | Obtido: $actual"

if [ "$actual" -eq "$expected" ]; then
    echo "✓ OK: contagem de linhas confere."
    echo "Test completed successfully!"
    exit 0
else
    echo "✗ FAIL: contagem de linhas não confere."
    exit 1
fi