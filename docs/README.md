# JobFleet - v3-final (v1-logging, v2-cli)

Biblioteca de logging thread-safe para o sistema de processamento distribuído de jobs JobFleet.

Tag: v3-final — Entrega final contendo documentação consolidada (diagrama de sequência, mapeamento requisitos → código e análise com IA). Consulte a seção "Relatório Final" abaixo.

## Início Rápido

### 1. Build do projeto

```bash
cmake -S . -B build
cmake --build build -j
```

### 2. Executar o teste

```bash
./scripts/stress_log.sh
```

### 3. Cliente/Servidor (v2-cli)

Servidor de chat mínimo (TCP) com logging `libtslog` e cliente CLI.

Build rápido via Makefile:

```bash
make -j
```

Subir servidor:

```bash
./build/chat_server --port 9000 --level info
```

Conectar cliente interativo:

```bash
./build/chat_client --host 127.0.0.1 --port 9000
# digite mensagens e ENTER para enviar
```

Clientes para teste não interativo:

```bash
./build/chat_client --host 127.0.0.1 --port 9000 \
  --count 5 --message "hello" --delay 10
```

Script de simulação de múltiplos clientes:

```bash
chmod +x scripts/test_chat.sh
./scripts/test_chat.sh   # usa porta 9100 por padrão
```

### 4. Teste manual (v1)

```bash
# Executar com parâmetros personalizados
./build/logtest --threads 4 --lines 10000 --out logs/custom.log

# Verificar contagem de linhas
wc -l logs/custom.log
```

## Funcionalidades

- **Thread-safe**: Fila MPSC (Multi-Producer Single-Consumer) com thread dedicada de escrita
- **Timestamps ISO-8601**: Formato UTC com precisão de milissegundos
- **Rastreamento de processo/thread**: Inclui PID e ID da thread em cada entrada de log
- **Rotação de logs**: Rotação opcional baseada no tamanho do arquivo
- **Múltiplas saídas**: Logging simultâneo em arquivo e stdout
- **Zero dependências**: Nenhuma biblioteca externa necessária

## Formato do Log

```
[YYYY-MM-DDThh:mm:ss.sssZ pid tid LEVEL] mensagem
```

Exemplo:
```
[2025-01-15T14:30:25.123Z 12345 0x1a2b3c4d INFO] thread=0 line=42
```

## Relatório Final

Consulte `docs/RELATORIO_FINAL.md` para:
- Diagrama de sequência cliente-servidor
- Mapeamento requisitos → código
- Análise técnica (IA) e próximos passos
