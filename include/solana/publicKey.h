#pragma once
#include <array>
#include <string>
#include <vector>
#include <cstdint>
namespace solana {
constexpr size_t PUBLIC_KEY_LENGTH = 32;
class PublicKey {
public:
  PublicKey() { _bytes.fill(0); }
  explicit PublicKey(const std::string &base58);
  explicit PublicKey(const std::array<uint8_t,PUBLIC_KEY_LENGTH> &bytes);
  explicit PublicKey(const std::vector<uint8_t> &bytes);
  explicit PublicKey(uint64_t value);
  bool equals(const PublicKey &other) const;
  std::string toBase58() const;
  std::string toJSON() const;
  std::vector<uint8_t> toBytes() const;
  std::array<uint8_t,PUBLIC_KEY_LENGTH> toBuffer() const;
  std::string toString() const;
  static PublicKey unique();
  static bool isOnCurve(const PublicKey &key);
  static const PublicKey Default;
private:
  std::array<uint8_t,PUBLIC_KEY_LENGTH> _bytes;
};
} // namespace solana
