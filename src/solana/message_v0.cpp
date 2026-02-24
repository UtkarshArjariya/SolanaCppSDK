#include "solana/message_v0.h"
#include "solana/base58.h"
#include <stdexcept>

namespace solana {

// ── Compact-u16 helper (same as legacy message) ──────────────────────────────

static void writeCompactU16(std::vector<uint8_t> &buf, uint16_t v) {
    do {
        uint8_t b = v & 0x7F;
        v >>= 7;
        if (v > 0)
            b |= 0x80;
        buf.push_back(b);
    } while (v > 0);
}

// ── MessageV0::compile ────────────────────────────────────────────────────────

MessageV0 MessageV0::compile(
    const std::vector<Instruction> &instructions,
    const PublicKey &feePayer,
    const std::string &recentBlockhash,
    const std::vector<AddressLookupTableAccount> &lookupTables)
{
    MessageV0 msg;
    msg._recentBlockhash = recentBlockhash;

    // ── Collect all keys needed, mark signer/writable status ─────────────────
    //
    // Strategy:
    //   1. Gather the "required" set: feePayer, all signers from instructions,
    //      all program IDs.  These MUST be static — they participate in
    //      signature verification or are program invocations.
    //   2. For remaining (non-signer, non-program) accounts, check if they
    //      appear in any provided lookup table.  If yes, record a lookup
    //      reference.  If no, add them to the static list.
    //   3. Build the static account ordering:
    //        [writable signers] [readonly signers] [writable non-signers] [readonly non-signers]
    //   4. The combined "virtual" account list for instruction compilation is:
    //        static_accounts ++ lookup_writable_accounts ++ lookup_readonly_accounts

    struct AccountInfo {
        PublicKey key;
        bool isSigner;
        bool isWritable;
    };

    // --- Pass 1: collect all static accounts ---------------------------------
    std::vector<AccountInfo> staticAccounts;
    // Helper: find-or-insert into staticAccounts, upgrading flags
    auto findOrAddStatic = [&](const PublicKey &pk, bool signer, bool writable) {
        for (auto &a : staticAccounts) {
            if (a.key.equals(pk)) {
                a.isSigner = a.isSigner || signer;
                a.isWritable = a.isWritable || writable;
                return;
            }
        }
        staticAccounts.push_back({pk, signer, writable});
    };

    // Fee payer: first signer, writable
    findOrAddStatic(feePayer, true, true);

    // Helper: is a key a signer in any instruction?
    // Pre-compute the set of signing pubkeys
    std::vector<PublicKey> signerSet;
    signerSet.push_back(feePayer);
    for (auto &instr : instructions) {
        for (auto &am : instr.accounts) {
            if (am.isSigner) {
                bool found = false;
                for (auto &s : signerSet)
                    if (s.equals(am.pubkey)) { found = true; break; }
                if (!found)
                    signerSet.push_back(am.pubkey);
            }
        }
    }

    auto isSignerKey = [&](const PublicKey &pk) -> bool {
        for (auto &s : signerSet)
            if (s.equals(pk)) return true;
        return false;
    };

    // Helper: search lookup tables for a pubkey (non-signer only)
    // Returns {tableIndex, addressIndex, isWritable} or {-1,...} if not found
    struct LookupRef { int tableIdx; uint8_t addrIdx; bool isWritable; };
    auto findInLookup = [&](const PublicKey &pk, bool writable) -> LookupRef {
        for (size_t ti = 0; ti < lookupTables.size(); ++ti) {
            auto &tbl = lookupTables[ti];
            for (size_t ai = 0; ai < tbl.addresses.size(); ++ai) {
                if (tbl.addresses[ai].equals(pk)) {
                    return { static_cast<int>(ti), static_cast<uint8_t>(ai), writable };
                }
            }
        }
        return { -1, 0, false };
    };

    // Prepare per-table writable / readonly index buckets
    struct TableBucket {
        std::vector<uint8_t> writableIdx;
        std::vector<uint8_t> readonlyIdx;
    };
    std::vector<TableBucket> tableBuckets(lookupTables.size());

    // Track which (tableIdx, addrIdx) pairs we have already registered so we
    // don't add duplicates to the lookup section
    struct LookupKey { int tableIdx; uint8_t addrIdx; bool isWritable; };
    std::vector<LookupKey> registeredLookups;
    auto findOrAddLookup = [&](int ti, uint8_t ai, bool writable) {
        for (auto &lk : registeredLookups)
            if (lk.tableIdx == ti && lk.addrIdx == ai) {
                // upgrade writability if needed
                if (writable && !lk.isWritable) lk.isWritable = true;
                return;
            }
        registeredLookups.push_back({ti, ai, writable});
        if (writable)
            tableBuckets[ti].writableIdx.push_back(ai);
        else
            tableBuckets[ti].readonlyIdx.push_back(ai);
    };

    // Process each instruction
    for (auto &instr : instructions) {
        // Program ID — always static, non-signer, non-writable
        findOrAddStatic(instr.programId, false, false);

        for (auto &am : instr.accounts) {
            if (am.isSigner || lookupTables.empty()) {
                // Signers must be static
                findOrAddStatic(am.pubkey, am.isSigner, am.isWritable);
            } else {
                // Try placing in a lookup table
                auto ref = findInLookup(am.pubkey, am.isWritable);
                if (ref.tableIdx >= 0) {
                    findOrAddLookup(ref.tableIdx, ref.addrIdx, am.isWritable);
                } else {
                    findOrAddStatic(am.pubkey, am.isSigner, am.isWritable);
                }
            }
        }
    }

    // ── Sort static accounts into the canonical Solana order ─────────────────
    //   [writable signers] [readonly signers] [writable non-signers] [readonly non-signers]
    std::vector<AccountInfo> ordered;
    // 1. Writable signers
    for (auto &a : staticAccounts)
        if (a.isSigner && a.isWritable) ordered.push_back(a);
    // 2. Readonly signers
    for (auto &a : staticAccounts)
        if (a.isSigner && !a.isWritable) ordered.push_back(a);
    // 3. Writable non-signers
    for (auto &a : staticAccounts)
        if (!a.isSigner && a.isWritable) ordered.push_back(a);
    // 4. Readonly non-signers
    for (auto &a : staticAccounts)
        if (!a.isSigner && !a.isWritable) ordered.push_back(a);

    // Build header counts
    uint8_t numSig = 0, numReadonlySig = 0, numReadonlyUnsig = 0;
    for (auto &a : ordered) {
        if (a.isSigner) {
            ++numSig;
            if (!a.isWritable) ++numReadonlySig;
        } else {
            if (!a.isWritable) ++numReadonlyUnsig;
        }
    }
    msg._header.numRequiredSignatures = numSig;
    msg._header.numReadonlySignedAccounts = numReadonlySig;
    msg._header.numReadonlyUnsignedAccounts = numReadonlyUnsig;

    for (auto &a : ordered)
        msg._staticAccountKeys.push_back(a.key);

    // ── Build virtual combined key list for instruction index resolution ──────
    // Combined = static accounts ++ lookup writable ++ lookup readonly
    struct VirtualKey { PublicKey key; };
    std::vector<VirtualKey> virtualKeys;
    for (auto &a : ordered)
        virtualKeys.push_back({a.key});
    for (size_t ti = 0; ti < lookupTables.size(); ++ti) {
        for (uint8_t idx : tableBuckets[ti].writableIdx)
            virtualKeys.push_back({lookupTables[ti].addresses[idx]});
    }
    for (size_t ti = 0; ti < lookupTables.size(); ++ti) {
        for (uint8_t idx : tableBuckets[ti].readonlyIdx)
            virtualKeys.push_back({lookupTables[ti].addresses[idx]});
    }

    // Resolve index of a key in the virtual list
    auto resolveIndex = [&](const PublicKey &pk) -> uint8_t {
        for (size_t i = 0; i < virtualKeys.size(); ++i)
            if (virtualKeys[i].key.equals(pk))
                return static_cast<uint8_t>(i);
        throw std::runtime_error("MessageV0::compile — account not found in virtual key list");
    };

    // ── Compile instructions ──────────────────────────────────────────────────
    for (auto &instr : instructions) {
        MessageV0::CompiledInstruction ci;
        ci.programIdIndex = resolveIndex(instr.programId);
        for (auto &am : instr.accounts)
            ci.accountIndices.push_back(resolveIndex(am.pubkey));
        ci.data = instr.data;
        msg._instructions.push_back(std::move(ci));
    }

    // ── Build address table lookup section ────────────────────────────────────
    for (size_t ti = 0; ti < lookupTables.size(); ++ti) {
        if (tableBuckets[ti].writableIdx.empty() && tableBuckets[ti].readonlyIdx.empty())
            continue;
        MessageAddressTableLookup entry;
        entry.accountKey = lookupTables[ti].key;
        entry.writableIndexes = tableBuckets[ti].writableIdx;
        entry.readonlyIndexes = tableBuckets[ti].readonlyIdx;
        msg._addressTableLookups.push_back(std::move(entry));
    }

    return msg;
}

// ── MessageV0::serialize ─────────────────────────────────────────────────────
// Serializes the message body WITHOUT the 0x80 version prefix.
// TransactionV0::serialize() prepends the prefix byte.

std::vector<uint8_t> MessageV0::serialize() const {
    std::vector<uint8_t> buf;
    buf.reserve(256); // typical V0 message

    // Header (3 bytes)
    buf.push_back(_header.numRequiredSignatures);
    buf.push_back(_header.numReadonlySignedAccounts);
    buf.push_back(_header.numReadonlyUnsignedAccounts);

    // Static account keys
    writeCompactU16(buf, static_cast<uint16_t>(_staticAccountKeys.size()));
    for (auto &pk : _staticAccountKeys) {
        auto b = pk.toBytes();
        buf.insert(buf.end(), b.begin(), b.end());
    }

    // Recent blockhash (32 bytes)
    auto bhBytes = Base58::decode(_recentBlockhash);
    if (bhBytes.size() != 32)
        throw std::runtime_error("MessageV0: invalid blockhash length");
    buf.insert(buf.end(), bhBytes.begin(), bhBytes.end());

    // Compiled instructions
    writeCompactU16(buf, static_cast<uint16_t>(_instructions.size()));
    for (auto &ci : _instructions) {
        buf.push_back(ci.programIdIndex);
        writeCompactU16(buf, static_cast<uint16_t>(ci.accountIndices.size()));
        buf.insert(buf.end(), ci.accountIndices.begin(), ci.accountIndices.end());
        writeCompactU16(buf, static_cast<uint16_t>(ci.data.size()));
        buf.insert(buf.end(), ci.data.begin(), ci.data.end());
    }

    // Address table lookup section
    writeCompactU16(buf, static_cast<uint16_t>(_addressTableLookups.size()));
    for (auto &alt : _addressTableLookups) {
        // 32-byte account key
        auto kb = alt.accountKey.toBytes();
        buf.insert(buf.end(), kb.begin(), kb.end());
        // Writable indexes
        writeCompactU16(buf, static_cast<uint16_t>(alt.writableIndexes.size()));
        buf.insert(buf.end(), alt.writableIndexes.begin(), alt.writableIndexes.end());
        // Readonly indexes
        writeCompactU16(buf, static_cast<uint16_t>(alt.readonlyIndexes.size()));
        buf.insert(buf.end(), alt.readonlyIndexes.begin(), alt.readonlyIndexes.end());
    }

    return buf;
}

} // namespace solana
