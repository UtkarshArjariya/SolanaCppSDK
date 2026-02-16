#include <cstring>
#include <sodium.h>
#include <solana/base58.h>
#include <solana/publicKey.h>
#include <stdexcept>

// We use libsodium's crypto_hash_sha256 for PDA derivation
#include <sodium/crypto_hash_sha256.h>

bool bytesAreCurvePoint(
    const std::array<uint8_t, crypto_core_ed25519_BYTES> &bytes) {
  return crypto_core_ed25519_is_valid_point(bytes.data()) != 0;
}

namespace solana {

const PublicKey PublicKey::Default =
    PublicKey(std::array<uint8_t, PUBLIC_KEY_LENGTH>{});

// ── Constructors ──────────────────────────────────────────────────────────

PublicKey::PublicKey(uint64_t value) {
  _bytes.fill(0);
  for (int i = 0; i < 8; i++) {
    _bytes[PUBLIC_KEY_LENGTH - 1 - i] = static_cast<uint8_t>(value >> (8 * i));
  }
}

PublicKey::PublicKey(const std::string &base58) {
  auto decoded = Base58::decode(base58);
  if (decoded.size() != PUBLIC_KEY_LENGTH) {
    throw std::runtime_error("Invalid public key input");
  }
  std::memcpy(_bytes.data(), decoded.data(), PUBLIC_KEY_LENGTH);
}

PublicKey::PublicKey(const std::vector<uint8_t> &bytes) {
  if (bytes.size() != PUBLIC_KEY_LENGTH) {
    throw std::runtime_error("Invalid public key length");
  }
  std::memcpy(_bytes.data(), bytes.data(), PUBLIC_KEY_LENGTH);
}

PublicKey::PublicKey(const std::array<uint8_t, PUBLIC_KEY_LENGTH> &bytes) {
  _bytes = bytes;
}

// ── Static methods ────────────────────────────────────────────────────────

PublicKey PublicKey::unique() {
  static uint64_t counter = 1;
  return PublicKey(counter++);
}

/**
 * Create an account address using a base public key, a seed string,
 * and a program ID.
 * SHA256(fromPublicKey || seed || programId)
 */
PublicKey PublicKey::createWithSeed(const PublicKey &fromPublicKey,
                                    const std::string &seed,
                                    const PublicKey &programId) {
  if (seed.size() > 32)
    throw std::runtime_error("Seed must be 32 bytes or less");

  crypto_hash_sha256_state state;
  crypto_hash_sha256_init(&state);

  auto fromBytes = fromPublicKey.toBuffer();
  crypto_hash_sha256_update(&state, fromBytes.data(), PUBLIC_KEY_LENGTH);
  crypto_hash_sha256_update(
      &state, reinterpret_cast<const uint8_t *>(seed.data()), seed.size());
  auto progBytes = programId.toBuffer();
  crypto_hash_sha256_update(&state, progBytes.data(), PUBLIC_KEY_LENGTH);

  std::array<uint8_t, 32> hash{};
  crypto_hash_sha256_final(&state, hash.data());

  return PublicKey(hash);
}

/**
 * Derive a program address from seeds and a program ID.
 * SHA256(seed[0] || seed[1] || ... || programId || "ProgramDerivedAddress")
 * The result must NOT be on the ed25519 curve.
 */
PublicKey PublicKey::createProgramAddressSync(
    const std::vector<std::vector<uint8_t>> &seeds,
    const PublicKey &programId) {
  // Validate seeds
  for (auto &seed : seeds) {
    if (seed.size() > 32)
      throw std::runtime_error("Seed exceeds 32 bytes");
  }

  crypto_hash_sha256_state state;
  crypto_hash_sha256_init(&state);

  // Hash all seeds
  for (auto &seed : seeds) {
    crypto_hash_sha256_update(&state, seed.data(), seed.size());
  }

  // Hash the program ID
  auto progBytes = programId.toBuffer();
  crypto_hash_sha256_update(&state, progBytes.data(), PUBLIC_KEY_LENGTH);

  // Hash the PDA marker string
  const char *marker = "ProgramDerivedAddress";
  crypto_hash_sha256_update(&state, reinterpret_cast<const uint8_t *>(marker),
                            21);

  std::array<uint8_t, 32> hash{};
  crypto_hash_sha256_final(&state, hash.data());

  // PDA must NOT be on the ed25519 curve
  if (crypto_core_ed25519_is_valid_point(hash.data()))
    throw std::runtime_error("Derived address is on the ed25519 curve");

  return PublicKey(hash);
}

PublicKey
PublicKey::createProgramAddress(const std::vector<std::vector<uint8_t>> &seeds,
                                const PublicKey &programId) {
  return createProgramAddressSync(seeds, programId);
}

/**
 * Find a valid program derived address (PDA) by searching for a bump
 * seed (255 down to 0) that produces an off-curve address.
 */
std::pair<PublicKey, uint8_t> PublicKey::findProgramAddressSync(
    const std::vector<std::vector<uint8_t>> &seeds,
    const PublicKey &programId) {
  for (int bump = 255; bump >= 0; --bump) {
    try {
      // Append the bump seed
      auto seedsWithBump = seeds;
      seedsWithBump.push_back({static_cast<uint8_t>(bump)});
      PublicKey address = createProgramAddressSync(seedsWithBump, programId);
      return {address, static_cast<uint8_t>(bump)};
    } catch (const std::runtime_error &) {
      // Address was on the curve, try next bump
      continue;
    }
  }
  throw std::runtime_error("Could not find a valid PDA");
}

std::pair<PublicKey, uint8_t>
PublicKey::findProgramAddress(const std::vector<std::vector<uint8_t>> &seeds,
                              const PublicKey &programId) {
  return findProgramAddressSync(seeds, programId);
}

bool PublicKey::isOnCurve(const PublicKey &key) {
  try {
    return crypto_core_ed25519_is_valid_point(key.toBuffer().data()) != 0;
  } catch (...) {
    return false;
  }
}

// ── Member methods ────────────────────────────────────────────────────────

bool PublicKey::equals(const PublicKey &other) const {
  return _bytes == other._bytes;
}

std::string PublicKey::toBase58() const {
  std::vector<unsigned char> vec(_bytes.begin(), _bytes.end());
  return Base58::encode(vec);
}

std::string PublicKey::toJSON() const { return toBase58(); }

std::vector<uint8_t> PublicKey::toBytes() const {
  return std::vector<uint8_t>(_bytes.begin(), _bytes.end());
}

std::array<uint8_t, PUBLIC_KEY_LENGTH> PublicKey::toBuffer() const {
  return _bytes;
}

std::string PublicKey::toString() const { return toBase58(); }

} // namespace solana
