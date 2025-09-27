# Architecture - v1-logging

## Overview

The libtslog library implements a thread-safe logging system using the MPSC (Multi-Producer Single-Consumer) pattern. This design ensures high performance by minimizing lock contention while guaranteeing thread safety.

## Core Design

The library uses a **dedicated writer thread** that consumes messages from a shared queue. Multiple producer threads can safely enqueue messages using a mutex-protected queue, while a single consumer thread handles all I/O operations asynchronously.

**Why MPSC?** This pattern eliminates the need for synchronization during file writes, reduces syscall overhead through batching, and provides better performance under high concurrency compared to per-thread file handles.

## Architecture Diagram

![Architecture Overview](diagrams/architecture.puml)

The diagram shows the current libtslog implementation and placeholder components for future JobFleet system development (Server, Worker, Enqueue).

## Implementation Details

- **Queue Management**: `std::deque` with `std::mutex` and `std::condition_variable`
- **Batched Writes**: Writer thread flushes messages in batches (every ~50ms or when buffer is full)
- **Log Rotation**: File size-based rotation with simple `.1` backup scheme
- **Error Handling**: Non-throwing API; errors logged to stderr without stopping service
- **Timestamps**: UTC with millisecond precision using `std::chrono`