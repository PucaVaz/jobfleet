#!/usr/bin/env bash
set -euo pipefail

# Configuração
PORT=${PORT:-9100}
CLIENTS=${CLIENTS:-5}
MSGS=${MSGS:-10}
DELAY_MS=${DELAY_MS:-5}

echo "Compilando projeto (Makefile)..."
make -j

mkdir -p logs
rm -f logs/chat_server.log logs/chat_client.log

echo "Iniciando chat_server na porta $PORT..."
./build/chat_server --port "$PORT" --log logs/chat_server.log --level info &
SERVER_PID=$!
sleep 0.2

cleanup() {
  echo "Encerrando server (pid=$SERVER_PID)..."
  kill -TERM "$SERVER_PID" >/dev/null 2>&1 || true
  wait "$SERVER_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

echo "Disparando $CLIENTS clientes, $MSGS mensagens cada..."
pids=()
for i in $(seq 1 "$CLIENTS"); do
  ./build/chat_client --host 127.0.0.1 --port "$PORT" \
    --count "$MSGS" --message "client=$i" --delay "$DELAY_MS" \
    --log logs/chat_client.log --level warn &
  pids+=("$!")
done

for p in "${pids[@]}"; do
  wait "$p"
done

sleep 0.5

echo "Validando logs do servidor..."
expected=$((CLIENTS*MSGS))
actual=$(grep -c "msg de" logs/chat_server.log || true)
echo "Esperado (linhas de msg de ...): $expected | Obtido: $actual"

if [ "$actual" -ge "$expected" ]; then
  echo "✓ OK: servidor processou mensagens."
else
  echo "✗ FAIL: contagem menor que o esperado."
  exit 1
fi

echo "Teste concluído. Logs em logs/chat_server.log"

