#pragma once
/**
 * Address Lookup Table (ALT) support for Versioned Transactions (V0).
 *
 * Address Lookup Tables are on-chain accounts that store lists of addresses.
 * V0 transactions can reference addresses inside these tables by index,
 * allowing up to 256 extra accounts per lookup table without transferring
 * the full 32-byte pubkeys over the wire.
 *
 * On MCUs, ALT accounts should be parsed once and cached in SRAM.
 * Each parsed entry is 32 bytes; a full table (256 entries) fits in 8 KB.
 */

#include "solana/publicKey.h"
#include <cstdint>
#include <vector>

namespace solana {

/**
 * A pre-parsed Address Lookup Table account.
 *
 * Obtain by calling RpcClient::getAddressLookupTable() or by manually
 * decoding the raw account data returned by getAccountInfo.
 */
struct AddressLookupTableAccount {
    PublicKey key;                   ///< The ALT account address on-chain
    std::vector<PublicKey> addresses; ///< Ordered addresses stored in this table

    /**
     * Parse raw ALT account data (already base64-decoded bytes).
     *
     * ALT account data layout (discriminator type=1):
     *   [4 B]  type tag             (must be 1)
     *   [8 B]  deactivation_slot    (u64 LE, u64::MAX means active)
     *   [8 B]  last_extended_slot   (u64 LE)
     *   [1 B]  last_extended_slot_start_index
     *   [1 B]  authority option flag (0 = None, 1 = Some)
     *   [32 B] authority pubkey     (only if flag == 1)
     *   remaining bytes — address entries, 32 bytes each
     *
     * @param accountKey  The public key of the ALT account itself
     * @param data        Raw bytes of the ALT account data
     * @param out         Output parsed ALT
     * @return true on success, false if data is malformed or not an ALT
     */
    static bool parse(const PublicKey &accountKey,
                      const std::vector<uint8_t> &data,
                      AddressLookupTableAccount &out);
};

} // namespace solana
