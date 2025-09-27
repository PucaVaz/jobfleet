# JobFleet - v1-logging

Biblioteca de logging thread-safe para o sistema de processamento distribuído de jobs JobFleet.

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

### 3. Teste manual

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