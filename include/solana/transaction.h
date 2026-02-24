#pragma once
#include "solana/keypair.h"
#include "solana/message.h"
#include "solana/message_v0.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace solana {

// ── Legacy Transaction ───────────────────────────────────────────────────────

/**
 * A signed Solana legacy transaction.
 * Compatible with all Solana validator versions.
 */
class Transaction {
public:
    explicit Transaction(const Message &message);
    void sign(const std::vector<Keypair> &signers);
    std::vector<uint8_t> serialize() const;
    std::string toBase64() const;
    const std::vector<std::array<uint8_t, SIGNATURE_LENGTH>> &signatures() const;

private:
    Message _message;
    std::vector<std::array<uint8_t, SIGNATURE_LENGTH>> _signatures;
};

// ── Versioned Transaction (V0) ───────────────────────────────────────────────

/**
 * A signed Solana V0 (versioned) transaction.
 *
 * V0 transactions support Address Lookup Tables (ALTs), allowing up to
 * 256 extra accounts per lookup table without transmitting full 32-byte
 * pubkeys in the transaction payload.
 *
 * Wire format:
 *   compact-u16  signature count
 *   signatures   (64 bytes each)
 *   0x80         version prefix byte  (marks this as a versioned transaction)
 *   message body (see MessageV0)
 *
 * Usage:
 *   auto msg  = MessageV0::compile(instructions, feePayer, blockhash, alts);
 *   auto txn  = TransactionV0(msg);
 *   txn.sign({signerKeypair});
 *   auto b64  = txn.toBase64();
 *   rpc.sendTransaction(b64);
 */
class TransactionV0 {
public:
    explicit TransactionV0(const MessageV0 &message);
    void sign(const std::vector<Keypair> &signers);
    std::vector<uint8_t> serialize() const;
    std::string toBase64() const;
    const std::vector<std::array<uint8_t, SIGNATURE_LENGTH>> &signatures() const;

private:
    MessageV0 _message;
    std::vector<std::array<uint8_t, SIGNATURE_LENGTH>> _signatures;
};

} // namespace solana

