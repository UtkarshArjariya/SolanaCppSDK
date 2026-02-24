/**
 * spl_token_transfer.cpp — Example: SPL Token transfer using the C++ SDK.
 *
 * Demonstrates:
 *   1. Deriving Associated Token Accounts (ATAs) for sender and recipient
 *   2. Building a transferChecked instruction for safety
 *   3. Optionally creating the recipient ATA if it doesn't exist
 *   4. Adding ComputeBudget priority fee instructions
 *   5. Compiling and signing a V0 transaction
 *   6. Sending via RPC
 *
 * Replace TOKEN_MINT, SENDER_SEED, and RECIPIENT with real values.
 *
 * Compile:
 *   cmake -B build && cmake --build build --target spl_token_transfer
 *
 * Run (devnet):
 *   ./build/spl_token_transfer
 */

#include "solana/keypair.h"
#include "solana/message_v0.h"
#include "solana/programs/compute_budget.h"
#include "solana/programs/spl_token.h"
#include "solana/programs/system_program.h"
#include "solana/rpc_client.h"
#include "solana/transaction.h"
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace solana;
using namespace solana::programs::splToken;
using namespace solana::programs::computeBudget;

int main() {
    // ── Configuration (replace these) ─────────────────────────────────────────

    // USDC devnet mint (6 decimals)
    PublicKey tokenMint(std::string("4zMMC9srt5Ri5X14GAgXhaHii3GnPAEERYPJgZJDncDU"));

    // Sender deterministic keypair (from seed)
    std::array<uint8_t, 32> senderSeed{};
    senderSeed[0] = 0xAB; senderSeed[1] = 0xCD;
    Keypair sender = Keypair::fromSeed(senderSeed);

    // Recipient wallet (they must have an ATA or we create one)
    PublicKey recipientWallet(
        std::string("Fg6PaFpoGXkYsidMpWTK6W2BeZ7FEfcYkg476zPFsLnS"));

    // Transfer amount in base units (1.5 USDC = 1,500,000 with 6 decimals)
    uint64_t transferAmount = 1'500'000;
    uint8_t tokenDecimals = 6;

    // ── 1. Derive ATAs ─────────────────────────────────────────────────────────

    auto [senderAta,   senderBump]    = findAssociatedTokenAddress(
        sender.publicKey(), tokenMint);
    auto [recipientAta, recipientBump] = findAssociatedTokenAddress(
        recipientWallet, tokenMint);

    std::cout << "Sender wallet:    " << sender.publicKey().toBase58() << "\n";
    std::cout << "Sender ATA:       " << senderAta.toBase58() << "\n";
    std::cout << "Recipient wallet: " << recipientWallet.toBase58() << "\n";
    std::cout << "Recipient ATA:    " << recipientAta.toBase58() << "\n";

    // ── 2. RPC ─────────────────────────────────────────────────────────────────
    RpcClient rpc("https://api.devnet.solana.com");

    // ── 3. Check if recipient ATA exists; create it if not ────────────────────
    std::string blockhash;
    try {
        blockhash = rpc.getLatestBlockhash();
        std::cout << "Blockhash: " << blockhash << "\n";
    } catch (const std::exception &e) {
        std::cerr << "getLatestBlockhash failed: " << e.what() << "\n";
        return 1;
    }

    bool recipientAtaExists = false;
    try {
        auto info = rpc.getAccountInfo(recipientAta);
        recipientAtaExists = info.exists;
    } catch (...) {}

    // ── 4. Build instruction list ──────────────────────────────────────────────
    std::vector<Instruction> instructions;

    // Priority fee (recommended for mainnet; safe on devnet)
    instructions.push_back(setComputeUnitLimit(100'000));
    instructions.push_back(setComputeUnitPrice(1'000));

    // Create recipient ATA if needed
    if (!recipientAtaExists) {
        std::cout << "Recipient ATA does not exist — adding createAssociatedTokenAccount\n";
        instructions.push_back(
            createAssociatedTokenAccount(
                sender.publicKey(), // payer
                recipientAta,
                recipientWallet,
                tokenMint));
    }

    // The actual token transfer (with decimal check for safety)
    instructions.push_back(
        transferChecked(
            senderAta,            // source token account
            tokenMint,            // mint
            recipientAta,         // destination token account
            sender.publicKey(),   // authority (owner of senderAta)
            transferAmount,
            tokenDecimals));

    // ── 5. Compile V0 message and sign ────────────────────────────────────────
    auto message = MessageV0::compile(instructions, sender.publicKey(), blockhash);
    std::cout << "Static accounts in V0 message: "
              << message.staticAccountKeys().size() << "\n";

    TransactionV0 tx(message);
    tx.sign({sender});

    auto encoded = tx.toBase64();
    std::cout << "Signed V0 tx (" << encoded.size() << " B base64)\n";

    // ── 6. Simulate first to check for errors  ────────────────────────────────
    std::cout << "Simulating transaction...\n";
    try {
        auto simResp = rpc.simulateTransaction(encoded, false);
        // A real app would parse simResp with JsonValue::parse and check the
        // "err" field before proceeding.
        std::cout << "Simulation response received (check err field for issues)\n";
    } catch (const std::exception &e) {
        std::cerr << "simulateTransaction error: " << e.what() << "\n";
        // Don't abort — the simulation endpoint may not be configured
    }

    // ── 7. Send ────────────────────────────────────────────────────────────────
    try {
        std::string sig = rpc.sendTransaction(encoded);
        std::cout << "SPL token transfer sent!\n";
        std::cout << "Signature: " << sig << "\n";
        std::cout << "Explorer:  https://explorer.solana.com/tx/" << sig
                  << "?cluster=devnet\n";

        bool confirmed = rpc.confirmTransaction(sig, 30000);
        std::cout << (confirmed ? "Confirmed!" : "Timed out.") << "\n";
    } catch (const std::exception &e) {
        std::cerr << "sendTransaction failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
