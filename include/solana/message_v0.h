#pragma once
/**
 * Versioned Transaction Message (V0).
 *
 * V0 messages extend legacy Solana messages with Address Lookup Table (ALT)
 * support.  Instead of embedding every account's 32-byte pubkey, a V0 message
 * can carry a compact list of (ALT address, writable-indexes, readonly-indexes)
 * triples.  The runtime resolves the actual pubkeys at execution time.
 *
 * Wire format for a V0 message (inside a versioned transaction):
 *
 *   Transaction envelope
 *     compact-u16  signature count
 *     signatures   (64 bytes each)
 *     0x80         version prefix  ← distinguishes v0 from legacy
 *   Message body
 *     [3 B]        header (numRequiredSigs, numReadonlySigned, numReadonlyUnsigned)
 *     compact-u16  static account key count
 *     [32 B each]  static account keys
 *     [32 B]       recent blockhash
 *     compact-u16  instruction count
 *     instructions (programIdIndex, acct indices, data — all compact-u16 encoded)
 *     compact-u16  address table lookup count
 *     lookups      (32B altKey, compact-u16 writable count, u8 indexes,
 *                   compact-u16 readonly count, u8 indexes)
 *
 * Static accounts include: feePayer + all signers + all accounts that come
 * from instructions that are NOT resolvable via a lookup table.
 * Accounts resolvable via ALTs appear only in the lookup section.
 *
 * MCU note: this class avoids dynamic allocation beyond the instruction
 * vectors that are already required for any Solana transaction.
 */

#include "solana/address_lookup_table.h"
#include "solana/instruction.h"
#include "solana/publicKey.h"
#include <cstdint>
#include <string>
#include <vector>

namespace solana {

/**
 * One entry in the address table lookup section of a V0 message.
 */
struct MessageAddressTableLookup {
    PublicKey accountKey;               ///< The ALT account address
    std::vector<uint8_t> writableIndexes; ///< Indices of writable accounts
    std::vector<uint8_t> readonlyIndexes; ///< Indices of readonly accounts
};

/**
 * A compiled V0 message ready for signing and transmission.
 *
 * Compile with MessageV0::compile(), then wrap in TransactionV0 to sign.
 */
class MessageV0 {
public:
    /**
     * Compile a V0 message from a list of instructions.
     *
     * When @p lookupTables is non-empty, the compiler tries to resolve each
     * non-signer account through the tables.  If found, it is replaced with
     * an ALT-relative lookup index rather than a static account entry.
     * Signers and program IDs are always kept as static entries.
     *
     * @param instructions     Instructions to include (order preserved)
     * @param feePayer         The fee payer (always static, always a signer)
     * @param recentBlockhash  Base58-encoded recent blockhash
     * @param lookupTables     Optional pre-parsed ALTs for address compression
     */
    static MessageV0 compile(
        const std::vector<Instruction> &instructions,
        const PublicKey &feePayer,
        const std::string &recentBlockhash,
        const std::vector<AddressLookupTableAccount> &lookupTables = {});

    /**
     * Serialize the message body to bytes (NO 0x80 version prefix here;
     * TransactionV0::serialize() prepends it).
     */
    std::vector<uint8_t> serialize() const;

    const std::vector<PublicKey> &staticAccountKeys() const {
        return _staticAccountKeys;
    }
    uint8_t numRequiredSignatures() const {
        return _header.numRequiredSignatures;
    }

private:
    struct Header {
        uint8_t numRequiredSignatures = 0;
        uint8_t numReadonlySignedAccounts = 0;
        uint8_t numReadonlyUnsignedAccounts = 0;
    };

    struct CompiledInstruction {
        uint8_t programIdIndex;
        std::vector<uint8_t> accountIndices;
        std::vector<uint8_t> data;
    };

    Header _header;
    std::vector<PublicKey> _staticAccountKeys;
    std::string _recentBlockhash;
    std::vector<CompiledInstruction> _instructions;
    std::vector<MessageAddressTableLookup> _addressTableLookups;
};

} // namespace solana
