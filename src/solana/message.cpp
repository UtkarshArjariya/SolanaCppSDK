#include "solana/message.h"
#include "solana/base58.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace solana {

// ── Compile ─────────────────────────────────────────────────────────────────

Message Message::compile(const std::vector<Instruction> &instructions,
                         const PublicKey &feePayer,
                         const std::string &recentBlockhash) {
  Message msg;
  msg._recentBlockhash = recentBlockhash;

  // Account key ordering: feePayer first, then all signers, then non-signers
  // We track: isSigner, isWritable flags per account
  std::vector<PublicKey> orderedKeys;
  std::vector<bool> isSigner;
  std::vector<bool> isWritable;

  auto findOrAdd = [&](const PublicKey &pk, bool signer,
                       bool writable) -> uint8_t {
    for (size_t i = 0; i < orderedKeys.size(); ++i) {
      if (orderedKeys[i].equals(pk)) {
        isSigner[i] = isSigner[i] || signer;
        isWritable[i] = isWritable[i] || writable;
        return static_cast<uint8_t>(i);
      }
    }
    orderedKeys.push_back(pk);
    isSigner.push_back(signer);
    isWritable.push_back(writable);
    return static_cast<uint8_t>(orderedKeys.size() - 1);
  };

  // Fee payer is always first signer + writable
  findOrAdd(feePayer, true, true);

  // Register all accounts from all instructions
  for (auto &instr : instructions) {
    for (auto &am : instr.accounts)
      findOrAdd(am.pubkey, am.isSigner, am.isWritable);
    findOrAdd(instr.programId, false, false);
  }

  // Count header fields
  uint8_t numSigners = 0;
  for (bool s : isSigner)
    if (s)
      numSigners++;
  uint8_t numReadonlySigned = 0;
  for (size_t i = 0; i < orderedKeys.size(); ++i)
    if (isSigner[i] && !isWritable[i])
      numReadonlySigned++;
  uint8_t numReadonlyUnsigned = 0;
  for (size_t i = 0; i < orderedKeys.size(); ++i)
    if (!isSigner[i] && !isWritable[i])
      numReadonlyUnsigned++;

  msg._header.numRequiredSignatures = numSigners;
  msg._header.numReadonlySignedAccounts = numReadonlySigned;
  msg._header.numReadonlyUnsignedAccounts = numReadonlyUnsigned;
  msg._accountKeys = orderedKeys;

  // Compile instructions to index-based form
  for (auto &instr : instructions) {
    CompiledInstruction ci;
    // Find program index
    for (size_t i = 0; i < orderedKeys.size(); ++i)
      if (orderedKeys[i].equals(instr.programId)) {
        ci.programIdIndex = static_cast<uint8_t>(i);
        break;
      }
    for (auto &am : instr.accounts)
      for (size_t i = 0; i < orderedKeys.size(); ++i)
        if (orderedKeys[i].equals(am.pubkey)) {
          ci.accountIndices.push_back(static_cast<uint8_t>(i));
          break;
        }
    ci.data = instr.data;
    msg._instructions.push_back(std::move(ci));
  }

  return msg;
}

// ── Serialize ───────────────────────────────────────────────────────────────
// Solana wire format:
//   [header: 3 bytes]
//   [account keys: compact-u16 count, then 32 bytes each]
//   [recent blockhash: 32 bytes]
//   [instructions: compact-u16 count, then each instruction]

static void writeCompactU16(std::vector<uint8_t> &buf, uint16_t v) {
  // Solana compact-u16: variable length 1-3 bytes
  do {
    uint8_t b = v & 0x7F;
    v >>= 7;
    if (v > 0)
      b |= 0x80;
    buf.push_back(b);
  } while (v > 0);
}

std::vector<uint8_t> Message::serialize() const {
  std::vector<uint8_t> buf;

  // Header
  buf.push_back(_header.numRequiredSignatures);
  buf.push_back(_header.numReadonlySignedAccounts);
  buf.push_back(_header.numReadonlyUnsignedAccounts);

  // Account keys
  writeCompactU16(buf, static_cast<uint16_t>(_accountKeys.size()));
  for (auto &pk : _accountKeys) {
    auto b = pk.toBytes();
    buf.insert(buf.end(), b.begin(), b.end());
  }

  // Recent blockhash (decode base58 → 32 bytes)
  auto bhBytes = Base58::decode(_recentBlockhash);
  if (bhBytes.size() != 32)
    throw std::runtime_error("Invalid blockhash length");
  buf.insert(buf.end(), bhBytes.begin(), bhBytes.end());

  // Instructions
  writeCompactU16(buf, static_cast<uint16_t>(_instructions.size()));
  for (auto &ci : _instructions) {
    buf.push_back(ci.programIdIndex);
    writeCompactU16(buf, static_cast<uint16_t>(ci.accountIndices.size()));
    buf.insert(buf.end(), ci.accountIndices.begin(), ci.accountIndices.end());
    writeCompactU16(buf, static_cast<uint16_t>(ci.data.size()));
    buf.insert(buf.end(), ci.data.begin(), ci.data.end());
  }

  return buf;
}

} // namespace solana
