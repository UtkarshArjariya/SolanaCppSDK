#include <solana/publicKey.h>
#include <solana/base58.h>
#include <stdexcept>
#include <cstring>
#include <sodium.h>

bool bytesAreCurvePoint(const std::array<uint8_t, crypto_core_ed25519_BYTES> &bytes)
{
  return crypto_core_ed25519_is_valid_point(bytes.data()) != 0;
}

namespace solana
{

  const PublicKey PublicKey::Default = PublicKey(
      std::array<uint8_t, PUBLIC_KEY_LENGTH>{});

  // Constructors
  PublicKey::PublicKey(uint64_t value)
  {
    _bytes.fill(0);
    // Store value in big endian into the last 8 bytes
    for (int i = 0; i < 8; i++)
    {
      _bytes[PUBLIC_KEY_LENGTH - 1 - i] = static_cast<uint8_t>(value >> (8 * i));
    }
  }

  PublicKey::PublicKey(const std::string &base58)
  {
    auto decoded = Base58::decode(base58);
    if (decoded.size() != PUBLIC_KEY_LENGTH)
    {
      throw std::runtime_error("Invalid public key input");
    }
    std::memcpy(_bytes.data(), decoded.data(), PUBLIC_KEY_LENGTH);
  }

  PublicKey::PublicKey(const std::vector<uint8_t> &bytes)
  {
    if (bytes.size() != PUBLIC_KEY_LENGTH)
    {
      throw std::runtime_error("Invalid public key length");
    }
    std::memcpy(_bytes.data(), bytes.data(), PUBLIC_KEY_LENGTH);
  }

  PublicKey::PublicKey(const std::array<uint8_t, PUBLIC_KEY_LENGTH> &bytes)
  {
    _bytes = bytes;
  }

  // Static methods
  PublicKey PublicKey::unique()
  {
    static uint64_t counter = 1;
    return PublicKey(counter++);
  }

  PublicKey PublicKey::createWithSeed(
      const PublicKey &fromPublicKey,
      const std::string &seed,
      const PublicKey &programId)
  {
    // TODO: implement SHA256(fromPublicKey || seed || programId)
    return PublicKey(std::array<uint8_t, PUBLIC_KEY_LENGTH>{});
  }

  PublicKey PublicKey::createProgramAddressSync(
      const std::vector<std::vector<uint8_t>> &seeds,
      const PublicKey &programId)
  {
    // TODO: implement (validate seeds, SHA256, isOnCurve check)
    return PublicKey(std::array<uint8_t, PUBLIC_KEY_LENGTH>{});
  }

  PublicKey PublicKey::createProgramAddress(
      const std::vector<std::vector<uint8_t>> &seeds,
      const PublicKey &programId)
  {
    return createProgramAddressSync(seeds, programId);
  }

  std::pair<PublicKey, uint8_t> PublicKey::findProgramAddressSync(
      const std::vector<std::vector<uint8_t>> &seeds,
      const PublicKey &programId)
  {
    // TODO: implement bump seed search (255..0)
    return {PublicKey(std::array<uint8_t, PUBLIC_KEY_LENGTH>{}), 255};
  }

  std::pair<PublicKey, uint8_t> PublicKey::findProgramAddress(
      const std::vector<std::vector<uint8_t>> &seeds,
      const PublicKey &programId)
  {
    return findProgramAddressSync(seeds, programId);
  }

  bool PublicKey::isOnCurve(const PublicKey &key)
  {
    try
    {
      return crypto_core_ed25519_is_valid_point(key.toBuffer().data()) != 0;
    }
    catch (...)
    {
      return false;
    }
  }

  // Member methods
  bool PublicKey::equals(const PublicKey &other) const
  {
    return _bytes == other._bytes;
  }

  std::string PublicKey::toBase58() const
  {
    std::vector<unsigned char> vec(_bytes.begin(), _bytes.end());
    return Base58::encode(vec);
  }

  std::string PublicKey::toJSON() const
  {
    return toBase58();
  }

  std::vector<uint8_t> PublicKey::toBytes() const
  {
    return std::vector<uint8_t>(_bytes.begin(), _bytes.end());
  }

  std::array<uint8_t, PUBLIC_KEY_LENGTH> PublicKey::toBuffer() const
  {
    return _bytes;
  }

  std::string PublicKey::toString() const
  {
    return toBase58();
  }

}
