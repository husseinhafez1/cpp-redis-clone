# cpp-redis-clone

> A high-performance, Redis-inspired in-memory key-value store written in modern C++ — demonstrating real-world systems engineering skills including protocol parsing, concurrency, durability, and observability. This project replicates the core functionality of Redis and is ideal for exploring custom data stores, embedded caches, or distributed job queues.

## Features

- **RESP Protocol** - Full implementation of Redis Serialization Protocol (RESP) with parser/serializer built from scratch
- **Thread-Safe Operations** - Concurrent command processing with Boost.Asio and mutex-protected memory operations
- **AOF Persistence** - Append-Only File logging with automatic replay on startup
- **TTL Support** - Key expiration with background cleanup thread
- **Prometheus Metrics** - Real-time monitoring of commands, memory usage, connections, and errors
- **Memory Tracking** - Precise byte-level memory usage monitoring
- **Python Test Client** - Integration testing with raw socket communication
- **Unit Testing** - Comprehensive test suite using Google Test framework

## Architecture

```
[Client] → [TCP Server Thread] → [RESP Parser] → [Command Handler] → [In-Memory Store]
                                                                    ↘ [AOF Log Writer]
                                                                    ↘ [Prometheus Metrics]
```

- Each client connection runs in its own thread
- TTL cleanup runs in background thread
- All operations update metrics and AOF in real-time
- Memory usage tracked at byte precision
- Thread-safe command processing

## Supported Commands

- `SET key value` - Set key to hold string value
- `GET key` - Get value of key
- `DEL key` - Delete key
- `EXPIRE key seconds` - Set key expiration time
- `TTL key` - Get time to live for key
- `PERSIST key` - Remove expiration from key
- `METRICS` - Get Prometheus-compatible metrics

## Quick Start

```bash
# Build
cd build
make

# Run
./cpp-redis-clone

# Test
python3 test_client.py

# Run Unit Tests
./tests/redis_tests
```

## Testing

### Unit Tests
The project uses Google Test framework for comprehensive unit testing:
- Store operations (SET, GET, DEL)
- TTL and expiration handling
- Memory usage tracking
- Thread safety verification
- AOF persistence

Example test output:
```bash
[==========] Running 8 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 8 tests from StoreTests
[ RUN      ] StoreTests.BasicOperations
[       OK ] StoreTests.BasicOperations (0 ms)
[ RUN      ] StoreTests.Expiry
[       OK ] StoreTests.Expiry (0 ms)
[ RUN      ] StoreTests.Persist
[       OK ] StoreTests.Persist (0 ms)
[ RUN      ] StoreTests.GetTTL
[       OK ] StoreTests.GetTTL (0 ms)
[----------] 8 tests from StoreTests (0 ms total)
[----------] Global test environment tear-down
[==========] 8 tests from 1 test suite ran. (0 ms total)
[  PASSED  ] 8 tests.
```

### Integration Tests
The Python test client provides integration testing:
- Full RESP protocol compliance
- Command sequence verification
- Metrics validation
- Memory tracking accuracy

## Metrics Example

```prometheus
redis_commands_total{command="set"} 1
redis_commands_total{command="get"} 2
redis_commands_total{command="del"} 1
redis_commands_total{command="persist"} 1
redis_commands_total{command="expire"} 1
redis_commands_total{command="ttl"} 1

redis_memory_bytes 120

redis_connections_active 1
redis_connections_total 10

redis_aof_writes_total 3
redis_aof_errors_total 0
```

## Integration Test Output

```bash
Testing SET command...
Response: +OK

Testing GET command...
Response: $5
value

Testing EXPIRE command...
Response: :1

Testing TTL command...
Response: :9
```

## Requirements

- C++17
- CMake 3.10+
- Boost
- Python 3
- Google Test