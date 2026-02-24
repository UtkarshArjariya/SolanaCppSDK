#pragma once
/**
 * SPL Token Program instructions.
 *
 * Mirrors the essential functionality of @solana/spl-token for C++.
 * This includes basic token transfers, minting, burning, and the
 * Associated Token Account (ATA) derivation logic.
 *
 * Program IDs:
 *   Token Program:      TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA
 *   Associated Token:   ATokenGPvbdGVxr1b2hvZbsiqW5xWH25efTNsLJe1bUs
 *
 * All instructions use Borsh-encoded data. Amounts are little-endian u64.
 *
 * MCU note: ATA derivation calls findProgramAddressSync which performs at most
 * 256 SHA256 hashes internally — this is acceptable even on Cortex-M4 class
 * MCUs (< 10ms). Each instruction object is heap-minimal.
 */

#include "solana/instruction.h"
#include "solana/publicKey.h"
#include <cstdint>
#include <utility>

namespace solana {
namespace programs {
namespace splToken {

// ── Program IDs ───────────────────────────────────────────────────────────────

static const char *const TOKEN_PROGRAM_ID =
    "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA";

static const char *const ASSOCIATED_TOKEN_PROGRAM_ID =
    "ATokenGPvbdGVxr1b2hvZbsiqW5xWH25efTNsLJe1bUs";

static const char *const SYSVAR_RENT_ID =
    "SysvarRent111111111111111111111111111111111";

inline PublicKey tokenProgramId() {
    return PublicKey(std::string(TOKEN_PROGRAM_ID));
}
inline PublicKey associatedTokenProgramId() {
    return PublicKey(std::string(ASSOCIATED_TOKEN_PROGRAM_ID));
}

// ── ATA derivation ────────────────────────────────────────────────────────────

/**
 * Derive the Associated Token Account (ATA) address for a given
 * wallet and token mint, WITHOUT checking the chain.
 *
 * Seeds: [walletAddress, tokenProgramId, mintAddress]
 * PDA:   findProgramAddressSync(seeds, associatedTokenProgramId)
 *
 * @param walletAddress    The owner's wallet public key
 * @param mintAddress      The SPL token mint public key
 * @return {ataAddress, bumpSeed}
 */
std::pair<PublicKey, uint8_t> findAssociatedTokenAddress(
    const PublicKey &walletAddress,
    const PublicKey &mintAddress);

// ── Token Instructions ────────────────────────────────────────────────────────

/**
 * transfer — discriminator 3.
 *
 * Transfer tokens from one account to another.
 * The source and destination must be token accounts for the same mint.
 * Does NOT verify decimals — use transferChecked for safety.
 *
 * Accounts: [source (writable), dest (writable), authority (signer)]
 *
 * @param source     Source token account (writable)
 * @param dest       Destination token account (writable)
 * @param authority  Owner/delegate of source account (signer)
 * @param amount     Amount in base units (e.g. lamports for SOL-wrapped tokens)
 */
Instruction transfer(const PublicKey &source,
                     const PublicKey &dest,
                     const PublicKey &authority,
                     uint64_t amount);

/**
 * transferChecked — discriminator 12.
 *
 * Transfer with decimal verification — the recommended way to transfer.
 * The validator checks that `decimals` matches the mint's decimals field,
 * preventing accidental off-by-one errors from mismatched decimal assumptions.
 *
 * Accounts: [source (writable), mint, dest (writable), authority (signer)]
 *
 * @param source    Source token account (writable)
 * @param mint      Token mint account
 * @param dest      Destination token account (writable)
 * @param authority Owner/delegate of source (signer)
 * @param amount    Amount in base units
 * @param decimals  Token decimal places (must match mint)
 */
Instruction transferChecked(const PublicKey &source,
                            const PublicKey &mint,
                            const PublicKey &dest,
                            const PublicKey &authority,
                            uint64_t amount,
                            uint8_t decimals);

/**
 * mintTo — discriminator 7.
 *
 * Mint new tokens into a token account.
 * Only the mint authority can call this.
 *
 * Accounts: [mint (writable), destination (writable), mintAuthority (signer)]
 *
 * @param mint          The token mint (writable — supply increases)
 * @param dest          Destination token account
 * @param mintAuthority The mint authority keypair (signer)
 * @param amount        Number of tokens to mint (in base units)
 */
Instruction mintTo(const PublicKey &mint,
                   const PublicKey &dest,
                   const PublicKey &mintAuthority,
                   uint64_t amount);

/**
 * burn — discriminator 8.
 *
 * Burn (destroy) tokens from a token account.
 *
 * Accounts: [account (writable), mint (writable), authority (signer)]
 *
 * @param account    Token account to burn from (writable)
 * @param mint       Token mint (writable — supply decreases)
 * @param authority  Owner/delegate of the token account (signer)
 * @param amount     Number of tokens to burn (base units)
 */
Instruction burn(const PublicKey &account,
                 const PublicKey &mint,
                 const PublicKey &authority,
                 uint64_t amount);

/**
 * closeAccount — discriminator 9.
 *
 * Close a token account and reclaim its rent lamports.
 * The token account must have a zero balance before closing.
 *
 * Accounts: [account (writable), destination (writable), authority (signer)]
 *
 * @param account     Token account to close (writable)
 * @param destination Where to return the rent lamports
 * @param authority   Owner of the token account (signer)
 */
Instruction closeAccount(const PublicKey &account,
                         const PublicKey &destination,
                         const PublicKey &authority);

/**
 * createAssociatedTokenAccountInstruction
 *
 * Build the instruction that creates an ATA via the Associated Token Program.
 * The ATA address is derived from payer's wallet and the token mint.
 *
 * Accounts:
 *   payer (writable, signer)
 *   ataAddress (writable)
 *   walletAddress
 *   mintAddress
 *   systemProgram
 *   tokenProgram
 *   sysvarRent
 *
 * @param payer         Pays for account creation (signer)
 * @param ataAddress    The derived ATA address (writable)
 * @param walletAddress The wallet that will own the ATA
 * @param mintAddress   The token mint
 */
Instruction createAssociatedTokenAccount(const PublicKey &payer,
                                         const PublicKey &ataAddress,
                                         const PublicKey &walletAddress,
                                         const PublicKey &mintAddress);

} // namespace splToken
} // namespace programs
} // namespace solana
