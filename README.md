# Solana C++ SDK

[![Build Status](https://github.com/utkarsharjariya/solanacppsdk/actions/workflows/cmake.yml/badge.svg)](https://github.com/utkarsharjariya/solanacppsdk/actions/workflows/cmake.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A lightweight C++ SDK for interacting with the Solana blockchain. Designed to work on both desktop systems and embedded/microcontroller platforms (ESP32, STM32, etc.).

## Features

*   **Keypairs** — Generate, import, sign, and verify Ed25519 keypairs via libsodium
*   **Transactions** — Construct, sign, and serialize Solana transactions
*   **RPC Client** — Full JSON-RPC client with HTTPS/TLS support (OpenSSL)
*   **HTTP Proxy** — Route requests through an HTTP proxy for embedded devices
*   **Base58** — Encode and decode Base58 (Solana addresses, signatures)
*   **BIP39** — Mnemonic phrase to seed derivation (PBKDF2-HMAC-SHA512)
*   **PDA** — Program Derived Address calculation (`findProgramAddress`, `createWithSeed`)
*   **Borsh** — Serialization helpers for instruction data
*   **System Program** — Built-in SOL transfer instruction helper
*   **Airdrop & Confirm** — `requestAirdrop()` and `confirmTransaction()` for devnet

## Getting Started

### Prerequisites

*   C++17 compatible compiler (GCC, Clang, MSVC)
*   CMake (version 3.10 or later)
*   libsodium (included as vendored headers + static lib)
*   OpenSSL (optional, for HTTPS/TLS support)

### Building

```bash
# Clone the repository
git clone --recurse-submodules https://github.com/UtkarshArjariya/SolanaCppSDK.git
cd SolanaCppSDK

# Build with OpenSSL (HTTPS support)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Build WITHOUT OpenSSL (use HTTP proxy instead)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSOLANA_SDK_NO_TLS=1
cmake --build build
```

### Quick Start — Offline Signing (Microcontroller)

```cpp
#include "solana/keypair.h"
#include "solana/instruction.h"
#include "solana/message.h"
#include "solana/transaction.h"
#include "solana/programs/system_program.h"

// Generate or load a keypair
auto signer = solana::Keypair::generate();

// Build a SOL transfer
auto ix = solana::programs::transfer(signer.publicKey(), recipient, 1000000);
auto msg = solana::Message::compile({ix}, signer.publicKey(), blockhash);
solana::Transaction tx(msg);
tx.sign({signer});

// Get base64-encoded signed transaction — ready to submit
std::string base64Tx = tx.toBase64();
```

### Quick Start — Full Flow (Desktop)

```cpp
#include "solana/rpc_client.h"
#include "solana/keypair.h"
#include "solana/transaction.h"
#include "solana/programs/system_program.h"

// Connect to devnet
solana::RpcClient rpc("https://api.devnet.solana.com");

// Generate keypair and get airdrop
auto kp = solana::Keypair::generate();
auto sig = rpc.requestAirdrop(kp.publicKey(), 2000000000);
rpc.confirmTransaction(sig);

// Transfer SOL
auto ix = solana::programs::transfer(kp.publicKey(), recipient, 500000000);
auto msg = solana::Message::compile({ix}, kp.publicKey(), rpc.getLatestBlockhash());
solana::Transaction tx(msg);
tx.sign({kp});
auto txSig = rpc.sendTransaction(tx.toBase64());
std::cout << "TX: " << txSig << std::endl;
```

### Using HTTP Proxy (for Embedded/MCU)

If your device doesn't support TLS, you can route through an HTTP proxy:

```bash
# On your gateway server, run a reverse proxy:
mitmdump --mode reverse:https://api.devnet.solana.com -p 8080
```

```cpp
solana::ProxyConfig proxy;
proxy.host = "192.168.1.100"; // Gateway IP
proxy.port = 8080;
proxy.enabled = true;

solana::RpcClient rpc("https://api.devnet.solana.com", proxy);
// All requests now go through the HTTP proxy — no TLS needed on device!
```

## Examples

| Example | Description |
|---------|-------------|
| `examples/transfer_sol.cpp` | End-to-end SOL transfer on devnet |
| `examples/offline_sign.cpp` | Offline signing for microcontrollers |
| `examples/pda_example.cpp` | Program Derived Address (PDA) derivation |

Run examples after building:
```bash
./build/transfer_sol
./build/offline_sign
./build/pda_example
```

## Project Structure

```
├── include/solana/       # Public headers
│   ├── base58.h          # Base58 encoding/decoding
│   ├── bip39.h           # BIP39 mnemonic → seed
│   ├── borsh.h           # Borsh serialization helpers
│   ├── instruction.h     # Instruction & AccountMeta types
│   ├── keypair.h         # Ed25519 keypair management
│   ├── message.h         # Transaction message compilation
│   ├── publicKey.h       # PublicKey with PDA support
│   ├── rpc_client.h      # JSON-RPC client (HTTP/HTTPS)
│   ├── transaction.h     # Transaction signing & serialization
│   └── programs/
│       └── system_program.h  # SOL transfer helper
├── src/solana/           # Implementation files
├── tests/solana/         # Unit tests
├── examples/             # Usage examples
├── scripts/              # Build & test scripts
├── include/sodium/       # Vendored libsodium headers
└── lib/                  # Pre-built libsodium
```

## Testing

```bash
# Run all tests
./scripts/run_tests.sh

# Run a specific test
./scripts/run_tests.sh test_base58

# Or via CMake
cd build && ctest --output-on-failure
```

## RPC Methods

| Method | Description |
|--------|-------------|
| `getLatestBlockhash()` | Get recent blockhash for signing |
| `sendTransaction()` | Submit a signed transaction |
| `getBalance()` | Get account balance in lamports |
| `getAccountInfo()` | Get raw account data |
| `requestAirdrop()` | Request devnet/testnet SOL |
| `confirmTransaction()` | Poll until transaction confirms |
| `getSignatureStatuses()` | Check transaction status |
| `getSlot()` | Get current slot number |

## Contributing

Contributions are welcome! Please feel free to open an issue or submit a pull request.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
