/**
 * v0_transaction.cpp — Example: Build and send a Versioned (V0) transaction.
 *
 * This example demonstrates:
 *   1. Fetching the latest blockhash
 *   2. Building a SOL transfer instruction
 *   3. Adding Compute Budget priority fee instructions
 *   4. Compiling a V0 MessageV0 (with optional ALT support)
 *   5. Signing with TransactionV0 and sending via RPC
 *
 * For MCU (offline signing) usage, only steps 2-4 need to run on-device.
 * Steps 1 and 5 run on a connected host / proxy server.
 *
 * Compile:
 *   cmake -B build && cmake --build build --target v0_transaction
 *
 * Run (devnet):
 *   ./build/v0_transaction
 */

#include "solana/keypair.h"
#include "solana/message_v0.h"
#include "solana/programs/compute_budget.h"
#include "solana/programs/system_program.h"
#include "solana/rpc_client.h"
#include "solana/transaction.h"
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace solana;

int main() {
    // ── 1. Keys ──────────────────────────────────────────────────────────────
    // In real MCU usage, load the seed from secure flash / TPM.
    std::array<uint8_t, 32> senderSeed{};
    senderSeed[0] = 0xAB; senderSeed[1] = 0xCD; // deterministic test key

    Keypair sender = Keypair::fromSeed(senderSeed);
    std::cout << "Sender:   " << sender.publicKey().toBase58() << "\n";

    // Hard-coded recipient (replace with real address)
    PublicKey recipient(std::string("Fg6PaFpoGXkYsidMpWTK6W2BeZ7FEfcYkg476zPFsLnS"));

    // ── 2. RPC client ─────────────────────────────────────────────────────────
    RpcClient rpc("https://api.devnet.solana.com");
    // For MCU via local HTTP proxy:
    //   ProxyConfig proxy{"192.168.1.100", 8080, true};
    //   RpcClient rpc("https://api.devnet.solana.com", proxy);

    // ── 3. Fetch latest blockhash ──────────────────────────────────────────────
    std::string blockhash;
    try {
        blockhash = rpc.getLatestBlockhash();
        std::cout << "Blockhash: " << blockhash << "\n";
    } catch (const std::exception &e) {
        std::cerr << "getLatestBlockhash failed: " << e.what() << "\n";
        return 1;
    }

    // ── 4. Build instructions ──────────────────────────────────────────────────

    // Priority fee — highly recommended for mainnet
    auto setLimit = programs::computeBudget::setComputeUnitLimit(50'000);
    auto setPrice = programs::computeBudget::setComputeUnitPrice(1'000); // 0.001 lamport/CU

    // Transfer 0.001 SOL (1,000,000 lamports)
    auto transfer = programs::transfer(sender.publicKey(), recipient, 1'000'000);

    std::vector<Instruction> instructions = {setLimit, setPrice, transfer};

    // ── 5. Compile V0 message ──────────────────────────────────────────────────
    // Pass an empty ALT list here. To use ALTs:
    //   AddressLookupTableAccount alt;
    //   rpc.getAddressLookupTable(altKey, alt);
    //   MessageV0::compile(instructions, sender.publicKey(), blockhash, {alt});

    auto message = MessageV0::compile(instructions, sender.publicKey(), blockhash);

    // ── 6. Sign (MCU step — runs on the secure signing device) ────────────────
    TransactionV0 tx(message);
    tx.sign({sender});

    std::string encoded = tx.toBase64();
    std::cout << "Signed V0 tx (" << encoded.size() << " chars base64)\n";

    // ── 7. Send ────────────────────────────────────────────────────────────────
    try {
        std::string sig = rpc.sendTransaction(encoded);
        std::cout << "Transaction sent!\nSignature: " << sig << "\n";
        std::cout << "Explorer:  https://explorer.solana.com/tx/" << sig
                  << "?cluster=devnet\n";

        // Optionally confirm
        std::cout << "Confirming...\n";
        bool confirmed = rpc.confirmTransaction(sig, 30000);
        std::cout << (confirmed ? "Confirmed!" : "Timed out waiting for confirmation.") << "\n";
    } catch (const std::exception &e) {
        std::cerr << "sendTransaction failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
