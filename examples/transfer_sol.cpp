/**
 * Solana C++ SDK — SOL Transfer Example
 *
 * This example demonstrates how to:
 * 1. Generate a keypair
 * 2. Request an airdrop on devnet
 * 3. Build a SOL transfer transaction
 * 4. Sign and send the transaction
 *
 * Usage:
 *   ./transfer_sol
 *
 * Note: This connects to Solana devnet. Make sure you have network
 *       access and OpenSSL installed for HTTPS support.
 *       Alternatively, run a local proxy:
 *         mitmdump --mode reverse:https://api.devnet.solana.com -p 8080
 *       Then change the endpoint to "http://localhost:8080"
 */

#include "solana/instruction.h"
#include "solana/keypair.h"
#include "solana/message.h"
#include "solana/programs/system_program.h"
#include "solana/rpc_client.h"
#include "solana/transaction.h"
#include <iomanip>
#include <iostream>

int main() {
  try {
    std::cout << "=== Solana C++ SDK — SOL Transfer Example ===" << std::endl;
    std::cout << std::endl;

    // ── Step 1: Generate keypairs ────────────────────────────────────────
    auto sender = solana::Keypair::generate();
    auto receiver = solana::Keypair::generate();

    std::cout << "Sender:   " << sender.publicKey().toBase58() << std::endl;
    std::cout << "Receiver: " << receiver.publicKey().toBase58() << std::endl;
    std::cout << std::endl;

    // ── Step 2: Connect to devnet ────────────────────────────────────────
    // Use HTTPS directly (requires OpenSSL):
    solana::RpcClient rpc("https://api.devnet.solana.com");

    // Or use an HTTP proxy:
    // solana::ProxyConfig proxy;
    // proxy.host = "localhost";
    // proxy.port = 8080;
    // proxy.enabled = true;
    // solana::RpcClient rpc("https://api.devnet.solana.com", proxy);

    std::cout << "Connected to Solana devnet" << std::endl;

    // ── Step 3: Request airdrop ──────────────────────────────────────────
    uint64_t airdropAmount = 2000000000; // 2 SOL in lamports
    std::cout << "Requesting airdrop of 2 SOL..." << std::endl;
    std::string airdropSig =
        rpc.requestAirdrop(sender.publicKey(), airdropAmount);
    std::cout << "Airdrop tx: " << airdropSig << std::endl;

    // Wait for confirmation
    std::cout << "Waiting for airdrop confirmation..." << std::endl;
    if (rpc.confirmTransaction(airdropSig, 30000)) {
      std::cout << "Airdrop confirmed!" << std::endl;
    } else {
      std::cerr << "Airdrop confirmation timed out" << std::endl;
      return 1;
    }

    // Check balance
    uint64_t balance = rpc.getBalance(sender.publicKey());
    std::cout << "Sender balance: " << balance / 1e9 << " SOL" << std::endl;
    std::cout << std::endl;

    // ── Step 4: Build transfer transaction ───────────────────────────────
    uint64_t transferAmount = 500000000; // 0.5 SOL
    std::cout << "Transferring 0.5 SOL to receiver..." << std::endl;

    // Create the transfer instruction
    auto transferIx = solana::programs::transfer(
        sender.publicKey(), receiver.publicKey(), transferAmount);

    // Get latest blockhash
    std::string blockhash = rpc.getLatestBlockhash();
    std::cout << "Blockhash: " << blockhash << std::endl;

    // Compile and sign
    auto message =
        solana::Message::compile({transferIx}, sender.publicKey(), blockhash);
    solana::Transaction tx(message);
    tx.sign({sender});

    // ── Step 5: Send transaction ─────────────────────────────────────────
    std::string txSig = rpc.sendTransaction(tx.toBase64());
    std::cout << "Transaction sent!" << std::endl;
    std::cout << "Signature: " << txSig << std::endl;
    std::cout << "Explorer:  https://explorer.solana.com/tx/" << txSig
              << "?cluster=devnet" << std::endl;

    // Wait for confirmation
    std::cout << "Waiting for confirmation..." << std::endl;
    if (rpc.confirmTransaction(txSig, 30000)) {
      std::cout << "Transaction confirmed!" << std::endl;
    } else {
      std::cerr << "Confirmation timed out (tx may still confirm later)"
                << std::endl;
    }

    // ── Step 6: Check final balances ─────────────────────────────────────
    std::cout << std::endl;
    std::cout << "Final balances:" << std::endl;
    std::cout << "  Sender:   " << rpc.getBalance(sender.publicKey()) / 1e9
              << " SOL" << std::endl;
    std::cout << "  Receiver: " << rpc.getBalance(receiver.publicKey()) / 1e9
              << " SOL" << std::endl;

    std::cout << std::endl;
    std::cout << "Done!" << std::endl;
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
