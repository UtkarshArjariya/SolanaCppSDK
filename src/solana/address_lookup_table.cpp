#include "solana/address_lookup_table.h"
#include <cstring>

namespace solana {

/**
 * Parse raw Address Lookup Table account data.
 *
 * Layout (reference: solana-program v1.x ALT state):
 *   [4 B]  discriminator / type tag    (must equal 1 for a valid ALT)
 *   [8 B]  deactivation_slot           u64 LE  (0xFFFFFFFFFFFFFFFF = active)
 *   [8 B]  last_extended_slot          u64 LE
 *   [1 B]  last_extended_slot_start_index  u8
 *   [1 B]  authority option flag       (0 = None, 1 = Some)
 *   [32 B] authority pubkey            (present only when flag == 1)
 *   remaining bytes — address entries, 32 bytes each
 *
 * Total header size WITHOUT authority  = 4+8+8+1+1 = 22 bytes
 * Total header size WITH authority     = 22+32      = 54 bytes
 */
bool AddressLookupTableAccount::parse(const PublicKey &accountKey,
                                      const std::vector<uint8_t> &data,
                                      AddressLookupTableAccount &out) {
    constexpr size_t HEADER_BASE = 22; // bytes before authority flag
    constexpr size_t AUTHORITY_SIZE = 32;

    if (data.size() < HEADER_BASE)
        return false;

    // Check discriminator == 1
    uint32_t typeTag = static_cast<uint32_t>(data[0])
                     | (static_cast<uint32_t>(data[1]) << 8)
                     | (static_cast<uint32_t>(data[2]) << 16)
                     | (static_cast<uint32_t>(data[3]) << 24);
    if (typeTag != 1)
        return false;

    // Authority option flag is at byte offset 21
    size_t addressStart = HEADER_BASE;
    if (data[21] == 1) {
        // Authority is present
        addressStart += AUTHORITY_SIZE;
    }

    if (data.size() < addressStart)
        return false;

    size_t remaining = data.size() - addressStart;
    if (remaining % PUBLIC_KEY_LENGTH != 0)
        return false; // corrupted data

    out.key = accountKey;
    out.addresses.clear();
    size_t numAddresses = remaining / PUBLIC_KEY_LENGTH;
    out.addresses.reserve(numAddresses);

    for (size_t i = 0; i < numAddresses; ++i) {
        std::vector<uint8_t> keyBytes(
            data.begin() + static_cast<ptrdiff_t>(addressStart + i * PUBLIC_KEY_LENGTH),
            data.begin() + static_cast<ptrdiff_t>(addressStart + (i + 1) * PUBLIC_KEY_LENGTH));
        out.addresses.emplace_back(keyBytes);
    }

    return true;
}

} // namespace solana
