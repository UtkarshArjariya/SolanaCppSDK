/**
 * Solana C++ SDK — PDA (Program Derived Address) Example
 *
 * Demonstrates finding PDAs, which are used extensively in Solana programs
 * for deterministic account addressing (e.g., token accounts, vaults, etc.)
 */

#include "solana/publicKey.h"
#include <iostream>
#include <vector>

int main() {
  try {
    std::cout << "=== Solana C++ SDK — PDA Example ===" << std::endl;
    std::cout << std::endl;

    // ── Example 1: Find PDA for a program ───────────────────────────────
    solana::PublicKey programId("11111111111111111111111111111111");

    std::vector<std::vector<uint8_t>> seeds = {
        {'h', 'e', 'l', 'l', 'o'},
    };

    auto [pda, bump] =
        solana::PublicKey::findProgramAddressSync(seeds, programId);
    std::cout << "PDA:  " << pda.toBase58() << std::endl;
    std::cout << "Bump: " << (int)bump << std::endl;
    std::cout << std::endl;

    // ── Example 2: PDA is NOT on the ed25519 curve ──────────────────────
    std::cout << "Is PDA on curve? "
              << (solana::PublicKey::isOnCurve(pda) ? "yes" : "no")
              << " (should be 'no')" << std::endl;
    std::cout << std::endl;

    // ── Example 3: createWithSeed ───────────────────────────────────────
    solana::PublicKey baseKey("11111111111111111111111111111111");
    auto derived =
        solana::PublicKey::createWithSeed(baseKey, "vault", programId);
    std::cout << "Derived (createWithSeed): " << derived.toBase58()
              << std::endl;
    std::cout << std::endl;

    // ── Example 4: Multi-seed PDA ───────────────────────────────────────
    solana::PublicKey userKey("11111111111111111111111111111111");
    std::vector<std::vector<uint8_t>> multiSeeds = {
        {'u', 's', 'e', 'r', '-', 'v', 'a', 'u', 'l', 't'},
        userKey.toBytes(),
    };

    auto [multiPda, multiBump] =
        solana::PublicKey::findProgramAddressSync(multiSeeds, programId);
    std::cout << "Multi-seed PDA: " << multiPda.toBase58() << std::endl;
    std::cout << "Multi-seed bump: " << (int)multiBump << std::endl;

    std::cout << std::endl;
    std::cout << "Done!" << std::endl;
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
