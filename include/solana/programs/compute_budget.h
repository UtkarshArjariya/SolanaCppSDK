#pragma once
/**
 * Solana Compute Budget Program instructions.
 *
 * Priority fees are essential on mainnet to avoid transaction drops.
 * These two instructions mirror the JS `ComputeBudgetProgram` class from
 * @solana/web3.js.
 *
 * Typical usage — add BEFORE your primary instruction:
 *
 *   using namespace solana::programs;
 *   auto setLimit = computeBudget::setComputeUnitLimit(200'000);
 *   auto setPrice = computeBudget::setComputeUnitPrice(1'000); // microLamports
 *   auto msg = Message::compile({setLimit, setPrice, transferIx}, feePayer, bh);
 *
 * Program ID: ComputeBudget111111111111111111111111111111
 *
 * MCU note: both instructions have a fixed, tiny data payload (5 or 9 bytes)
 * and zero account references, so they add minimal overhead to the transaction.
 */

#include "solana/instruction.h"
#include <cstdint>

namespace solana {
namespace programs {
namespace computeBudget {

/// Compute Budget program ID (base58)
static const char *const COMPUTE_BUDGET_PROGRAM_ID =
    "ComputeBudget111111111111111111111111111111";

/**
 * setComputeUnitLimit — instruction discriminator 2.
 *
 * Sets the maximum compute units the transaction may consume.
 * Default is 200,000 per instruction / 1,400,000 per transaction.
 *
 * Wire format: [0x02] [units: u32 LE]  (5 bytes total)
 *
 * @param units  Maximum compute units (recommended: 200_000 for simple txs)
 */
Instruction setComputeUnitLimit(uint32_t units);

/**
 * setComputeUnitPrice — instruction discriminator 3.
 *
 * Sets the priority fee per compute unit in micro-lamports.
 * Total priority fee = (units * microLamports) / 1,000,000 lamports
 *
 * Wire format: [0x03] [microLamports: u64 LE]  (9 bytes total)
 *
 * @param microLamports  Priority fee in micro-lamports per compute unit.
 *                       1 lamport = 1,000,000 micro-lamports.
 *                       Example: 1000 = 0.001 lamport per CU
 */
Instruction setComputeUnitPrice(uint64_t microLamports);

/// Convenience: return the Compute Budget program PublicKey
PublicKey computeBudgetProgramId();

} // namespace computeBudget
} // namespace programs
} // namespace solana
