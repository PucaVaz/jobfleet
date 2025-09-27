# JobFleet - v1-logging

Thread-safe logging library for the JobFleet distributed job processing system.

## Quick Start

### 1. Build the project

```bash
cmake -S . -B build
cmake --build build -j
```

### 2. Run the test

```bash
./scripts/stress_log.sh
```

### 3. Manual testing

```bash
# Run with custom parameters
./build/logtest --threads 4 --lines 10000 --out logs/custom.log

# Check line count
wc -l logs/custom.log
```

## Features

- **Thread-safe**: MPSC (Multi-Producer Single-Consumer) queue with dedicated writer thread
- **ISO-8601 timestamps**: UTC format with millisecond precision
- **Process/Thread tracking**: Includes PID and thread ID in each log entry
- **Log rotation**: Optional file size-based rotation
- **Multiple outputs**: Simultaneous file and stdout logging
- **Zero dependencies**: No external libraries required

## Log Format

```
[YYYY-MM-DDThh:mm:ss.sssZ pid tid LEVEL] message
```

Example:
```
[2025-01-15T14:30:25.123Z 12345 0x1a2b3c4d INFO] thread=0 line=42
```