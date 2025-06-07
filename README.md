# C++ Redis Clone

A high-performance, concurrent in-memory key-value store written in modern C++, inspired by Redis. This project is designed as a learning exercise to understand systems programming, concurrency, and distributed systems concepts.

## Learning Goals

- Modern C++ features and best practices
- Thread-safe concurrent programming
- Memory management and optimization
- Network programming and protocol design
- System design and architecture

## Core Features

1. **In-Memory Storage**
   - Thread-safe key-value operations
   - Efficient memory management
   - Expiration mechanism (TTL)

2. **Concurrency**
   - Multi-threaded command handling
   - Lock-free operations where possible
   - Thread-safe data structures

3. **Redis Protocol**
   - RESP (Redis Serialization Protocol) implementation
   - Basic command set (GET, SET, DEL, EXPIRE)
   - Extensible command framework

4. **Network Layer**
   - TCP server implementation
   - Connection management
   - Event-driven architecture

## Project Structure

```
.
├── include/               # Public headers
│   ├── store/            # Core storage engine
│   ├── protocol/         # Redis protocol implementation
│   └── utils/            # Utility functions
├── src/                  # Implementation files
│   ├── store/           # Storage engine
│   ├── protocol/        # Protocol implementation
│   └── utils/           # Utilities
└── tests/               # Unit tests
```

## Development Phases

### Phase 1: Core Storage Engine
- Basic key-value operations
- Thread-safe implementation
- Expiration mechanism

### Phase 2: Network Layer
- TCP server
- Connection handling
- Basic protocol implementation

### Phase 3: Advanced Features
- Persistence
- Pub/Sub
- Performance optimizations

## Building the Project

### Prerequisites
- C++17 compatible compiler
- CMake 3.15+
- Make or Ninja

### Build Instructions
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Learning Resources

- [Redis Protocol Specification](https://redis.io/topics/protocol)
- [C++ Concurrency in Action](https://www.manning.com/books/c-plus-plus-concurrency-in-action)
- [Modern C++ Features](https://github.com/AnthonyCalandra/modern-cpp-features)

## License

MIT License 