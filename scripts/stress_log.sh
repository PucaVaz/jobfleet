#!/usr/bin/env bash
set -euo pipefail

# Configuração
build_dir=build
threads=8
lines=50000

echo "Compilando projeto..."
make clean
make -j

echo "Limpando diretório de logs..."
rm -rf logs && mkdir -p logs

echo "Executando teste de stress com $threads threads, $lines linhas cada..."
./build/logtest --threads $threads --lines $lines --out logs/test.log

expected=$((threads*lines))
actual=$(wc -l < logs/test.log | tr -d ' ')

echo "Esperado: $expected | Obtido: $actual"

if [ "$actual" -eq "$expected" ]; then
    echo "✓ OK: contagem de linhas confere."
    echo "Teste concluído com sucesso!"
    exit 0
else
    echo "✗ FAIL: contagem de linhas não confere."
    exit 1
fi