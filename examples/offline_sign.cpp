/**
 * Solana C++ SDK — Offline Signing Example
 *
 * This example demonstrates how to sign a transaction OFFLINE,
 * which is the primary use case for microcontrollers (ESP32, STM32, etc.)
 *
 * The microcontroller:
 * 1. Holds the private key securely
 * 2. Receives a recent blockhash (from a gateway/server)
 * 3. Constructs and signs the transaction locally
 * 4. Returns the signed base64 transaction for the gateway to submit
 *
 * No network/TLS required on the microcontroller side!
 */

#include "solana/borsh.h"
#include "solana/instruction.h"
#include "solana/keypair.h"
#include "solana/message.h"
#include "solana/programs/system_program.h"
#include "solana/transaction.h"
#include <iomanip>
#include <iostream>

// Simulate receiving a blockhash from a gateway
static std::string simulateBlockhashFromGateway() {
  // In real use, the gateway server would fetch this and send to the MCU
  return "11111111111111111111111111111111";
}

int main() {
  try {
    std::cout << "=== Solana C++ SDK — Offline Signing Example ==="
              << std::endl;
    std::cout << std::endl;
    std::cout << "This demonstrates offline transaction signing" << std::endl;
    std::cout << "suitable for microcontrollers (ESP32, STM32, etc.)"
              << std::endl;
    std::cout << std::endl;

    // ── Step 1: Load keypair from seed ─────────────────────────────────
    // On a real MCU, the seed would be stored in secure flash/EEPROM
    std::array<uint8_t, 32> seed{};
    seed[0] = 0x01; // Example seed
    seed[31] = 0xFF;

    auto signer = solana::Keypair::fromSeed(seed);
    std::cout << "Signer public key: " << signer.publicKey().toBase58()
              << std::endl;

    // ── Step 2: Define the recipient ───────────────────────────────────
    solana::PublicKey recipient("So11111111111111111111111111111111111111112");
    std::cout << "Recipient: " << recipient.toBase58() << std::endl;

    // ── Step 3: Get blockhash from gateway ─────────────────────────────
    std::string blockhash = simulateBlockhashFromGateway();
    std::cout << "Blockhash: " << blockhash << std::endl;
    std::cout << std::endl;

    // ── Step 4: Build transfer instruction ─────────────────────────────
    uint64_t lamports = 100000; // 0.0001 SOL
    auto transferIx =
        solana::programs::transfer(signer.publicKey(), recipient, lamports);

    std::cout << "Transfer amount: " << lamports << " lamports ("
              << lamports / 1e9 << " SOL)" << std::endl;

    // ── Step 5: Compile message and sign ───────────────────────────────
    auto msg =
        solana::Message::compile({transferIx}, signer.publicKey(), blockhash);
    solana::Transaction tx(msg);
    tx.sign({signer});

    // ── Step 6: Get the signed transaction ─────────────────────────────
    std::string base64Tx = tx.toBase64();
    std::cout << std::endl;
    std::cout << "=== Signed Transaction (base64) ===" << std::endl;
    std::cout << base64Tx << std::endl;
    std::cout << std::endl;

    // Get raw bytes for debugging
    auto rawBytes = tx.serialize();
    std::cout << "Raw transaction size: " << rawBytes.size() << " bytes"
              << std::endl;
    std::cout << "First signature: ";
    for (int i = 0; i < 8; ++i) {
      printf("%02x", tx.signatures()[0][i]);
    }
    std::cout << "..." << std::endl;

    std::cout << std::endl;
    std::cout << "The gateway server would now submit this transaction"
              << std::endl;
    std::cout << "to the Solana RPC via sendTransaction()." << std::endl;

    return 0;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
