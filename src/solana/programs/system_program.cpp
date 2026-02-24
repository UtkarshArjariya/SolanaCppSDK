#include "solana/programs/system_program.h"
#include <cstring>

namespace solana {
namespace programs {

// Helper: write uint32_t little-endian
static void writeU32(std::vector<uint8_t> &buf, uint32_t v) {
  buf.push_back(v & 0xFF);
  buf.push_back((v >> 8) & 0xFF);
  buf.push_back((v >> 16) & 0xFF);
  buf.push_back((v >> 24) & 0xFF);
}

// Helper: write uint64_t little-endian
static void writeU64(std::vector<uint8_t> &buf, uint64_t v) {
  for (int i = 0; i < 8; ++i)
    buf.push_back(static_cast<uint8_t>(v >> (8 * i)));
}

// Helper: write Borsh string (u32 length prefix + bytes)
static void writeString(std::vector<uint8_t> &buf, const std::string &s) {
  writeU32(buf, static_cast<uint32_t>(s.size()));
  buf.insert(buf.end(), s.begin(), s.end());
}

static PublicKey sysProg() { return PublicKey(std::string(SYSTEM_PROGRAM_ID)); }

// Recent blockhashes sysvar: SysvarRecentB1telephones11111111111111111
static PublicKey recentBlockhashesSysvar() {
  return PublicKey(std::string("SysvarRecentB1telephones11111111111111111"));
}

// Rent sysvar
static PublicKey rentSysvar() {
  return PublicKey(std::string("SysvarRent111111111111111111111111111111111"));
}

Instruction transfer(const PublicKey &from, const PublicKey &to,
                     uint64_t lamports) {
  std::vector<uint8_t> data;
  writeU32(data, static_cast<uint32_t>(SystemInstruction::Transfer));
  writeU64(data, lamports);
  return Instruction(
      sysProg(), {AccountMeta(from, true, true), AccountMeta(to, false, true)},
      data);
}

Instruction createAccount(const PublicKey &fromPubkey,
                          const PublicKey &newAccountPubkey, uint64_t lamports,
                          uint64_t space, const PublicKey &programId) {
  std::vector<uint8_t> data;
  writeU32(data, static_cast<uint32_t>(SystemInstruction::CreateAccount));
  writeU64(data, lamports);
  writeU64(data, space);
  auto pid = programId.toBytes();
  data.insert(data.end(), pid.begin(), pid.end());
  return Instruction(sysProg(),
                     {AccountMeta(fromPubkey, true, true),
                      AccountMeta(newAccountPubkey, true, true)},
                     data);
}

Instruction assign(const PublicKey &accountPubkey, const PublicKey &programId) {
  std::vector<uint8_t> data;
  writeU32(data, static_cast<uint32_t>(SystemInstruction::Assign));
  auto pid = programId.toBytes();
  data.insert(data.end(), pid.begin(), pid.end());
  return Instruction(sysProg(), {AccountMeta(accountPubkey, true, true)}, data);
}

Instruction allocate(const PublicKey &accountPubkey, uint64_t space) {
  std::vector<uint8_t> data;
  writeU32(data, static_cast<uint32_t>(SystemInstruction::Allocate));
  writeU64(data, space);
  return Instruction(sysProg(), {AccountMeta(accountPubkey, true, true)}, data);
}

Instruction createAccountWithSeed(const PublicKey &fromPubkey,
                                  const PublicKey &newAccountPubkey,
                                  const PublicKey &basePubkey,
                                  const std::string &seed, uint64_t lamports,
                                  uint64_t space, const PublicKey &programId) {
  std::vector<uint8_t> data;
  writeU32(data,
           static_cast<uint32_t>(SystemInstruction::CreateAccountWithSeed));
  auto base = basePubkey.toBytes();
  data.insert(data.end(), base.begin(), base.end());
  writeString(data, seed);
  writeU64(data, lamports);
  writeU64(data, space);
  auto pid = programId.toBytes();
  data.insert(data.end(), pid.begin(), pid.end());
  return Instruction(sysProg(),
                     {AccountMeta(fromPubkey, true, true),
                      AccountMeta(newAccountPubkey, false, true)},
                     data);
}

Instruction transferWithSeed(const PublicKey &fromPubkey,
                             const PublicKey &fromBasePubkey,
                             const std::string &fromSeed,
                             const PublicKey &fromOwner,
                             const PublicKey &toPubkey, uint64_t lamports) {
  std::vector<uint8_t> data;
  writeU32(data, static_cast<uint32_t>(SystemInstruction::TransferWithSeed));
  writeU64(data, lamports);
  writeString(data, fromSeed);
  auto owner = fromOwner.toBytes();
  data.insert(data.end(), owner.begin(), owner.end());
  return Instruction(sysProg(),
                     {AccountMeta(fromPubkey, false, true),
                      AccountMeta(fromBasePubkey, true, false),
                      AccountMeta(toPubkey, false, true)},
                     data);
}

Instruction advanceNonceAccount(const PublicKey &nonceAccount,
                                const PublicKey &authorizedPubkey) {
  std::vector<uint8_t> data;
  writeU32(data, static_cast<uint32_t>(SystemInstruction::AdvanceNonceAccount));
  return Instruction(sysProg(),
                     {AccountMeta(nonceAccount, false, true),
                      AccountMeta(recentBlockhashesSysvar(), false, false),
                      AccountMeta(authorizedPubkey, true, false)},
                     data);
}

Instruction withdrawNonceAccount(const PublicKey &nonceAccount,
                                 const PublicKey &authorizedPubkey,
                                 const PublicKey &toPubkey, uint64_t lamports) {
  std::vector<uint8_t> data;
  writeU32(data,
           static_cast<uint32_t>(SystemInstruction::WithdrawNonceAccount));
  writeU64(data, lamports);
  return Instruction(sysProg(),
                     {AccountMeta(nonceAccount, false, true),
                      AccountMeta(toPubkey, false, true),
                      AccountMeta(recentBlockhashesSysvar(), false, false),
                      AccountMeta(rentSysvar(), false, false),
                      AccountMeta(authorizedPubkey, true, false)},
                     data);
}

Instruction initializeNonceAccount(const PublicKey &nonceAccount,
                                   const PublicKey &authorizedPubkey) {
  std::vector<uint8_t> data;
  writeU32(data,
           static_cast<uint32_t>(SystemInstruction::InitializeNonceAccount));
  auto auth = authorizedPubkey.toBytes();
  data.insert(data.end(), auth.begin(), auth.end());
  return Instruction(sysProg(),
                     {AccountMeta(nonceAccount, false, true),
                      AccountMeta(recentBlockhashesSysvar(), false, false),
                      AccountMeta(rentSysvar(), false, false)},
                     data);
}

Instruction authorizeNonceAccount(const PublicKey &nonceAccount,
                                  const PublicKey &authorizedPubkey,
                                  const PublicKey &newAuthorizedPubkey) {
  std::vector<uint8_t> data;
  writeU32(data,
           static_cast<uint32_t>(SystemInstruction::AuthorizeNonceAccount));
  auto newAuth = newAuthorizedPubkey.toBytes();
  data.insert(data.end(), newAuth.begin(), newAuth.end());
  return Instruction(sysProg(),
                     {AccountMeta(nonceAccount, false, true),
                      AccountMeta(authorizedPubkey, true, false)},
                     data);
}

} // namespace programs
} // namespace solana
