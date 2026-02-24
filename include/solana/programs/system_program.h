#pragma once
/**
 * Solana System Program instructions.
 * Mirrors @solana/web3.js SystemProgram class.
 */

#include "solana/instruction.h"
#include "solana/publicKey.h"
#include <cstdint>
#include <string>

namespace solana {
namespace programs {

/// System program ID: 11111111111111111111111111111111
static const char *const SYSTEM_PROGRAM_ID = "11111111111111111111111111111111";

/// System program instruction indices
enum class SystemInstruction : uint32_t {
  CreateAccount = 0,
  Assign = 1,
  Transfer = 2,
  CreateAccountWithSeed = 3,
  AdvanceNonceAccount = 4,
  WithdrawNonceAccount = 5,
  InitializeNonceAccount = 6,
  AuthorizeNonceAccount = 7,
  Allocate = 8,
  AllocateWithSeed = 9,
  AssignWithSeed = 10,
  TransferWithSeed = 11,
};

/// Transfer SOL from one account to another
Instruction transfer(const PublicKey &from, const PublicKey &to,
                     uint64_t lamports);

/// Create a new account
Instruction createAccount(const PublicKey &fromPubkey,
                          const PublicKey &newAccountPubkey, uint64_t lamports,
                          uint64_t space, const PublicKey &programId);

/// Assign an account to a program
Instruction assign(const PublicKey &accountPubkey, const PublicKey &programId);

/// Allocate space for an account
Instruction allocate(const PublicKey &accountPubkey, uint64_t space);

/// Create account with a seed
Instruction createAccountWithSeed(const PublicKey &fromPubkey,
                                  const PublicKey &newAccountPubkey,
                                  const PublicKey &basePubkey,
                                  const std::string &seed, uint64_t lamports,
                                  uint64_t space, const PublicKey &programId);

/// Transfer SOL with a seed-derived source
Instruction transferWithSeed(const PublicKey &fromPubkey,
                             const PublicKey &fromBasePubkey,
                             const std::string &fromSeed,
                             const PublicKey &fromOwner,
                             const PublicKey &toPubkey, uint64_t lamports);

// ── Nonce instructions ──────────────────────────────────────────────────

/// Advance a nonce account's stored nonce
Instruction advanceNonceAccount(const PublicKey &nonceAccount,
                                const PublicKey &authorizedPubkey);

/// Withdraw from a nonce account
Instruction withdrawNonceAccount(const PublicKey &nonceAccount,
                                 const PublicKey &authorizedPubkey,
                                 const PublicKey &toPubkey, uint64_t lamports);

/// Initialize a nonce account
Instruction initializeNonceAccount(const PublicKey &nonceAccount,
                                   const PublicKey &authorizedPubkey);

/// Change the authority of a nonce account
Instruction authorizeNonceAccount(const PublicKey &nonceAccount,
                                  const PublicKey &authorizedPubkey,
                                  const PublicKey &newAuthorizedPubkey);

/// Get the System Program PublicKey
inline PublicKey systemProgramId() {
  return PublicKey(std::string(SYSTEM_PROGRAM_ID));
}

} // namespace programs
} // namespace solana
