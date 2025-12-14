# Solana C++ SDK

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A C++ SDK for interacting with the Solana blockchain. This project is currently a work in progress.

## Features
- **Keypairs**: Generate, sign, and verify keypairs using libsodium
- **Encoding**: Base58 encode/decode

## Prerequisites
- C++17 compatible compiler
- CMake >= 3.10
- libsodium

## Building
```bash
cmake -B build
cmake --build build
```

## Usage
```cpp
#include "solana/keypair.h"
#include <iostream>
int main() {
    auto kp = solana::Keypair::generate();
    std::cout << "Public Key: " << kp.publicKey().toBase58() << std::endl;
}
```

## License
MIT License
