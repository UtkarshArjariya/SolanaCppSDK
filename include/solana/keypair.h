#pragma once
#include "solana/publicKey.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace solana {
constexpr size_t SECRET_KEY_LENGTH = 64;
constexpr size_t SIGNATURE_LENGTH = 64;
class Keypair {
public:
  static Keypair generate();
  static Keypair fromSecretKey(const std::array<uint8_t,SECRET_KEY_LENGTH> &sk);
  static Keypair fromSeed(const std::array<uint8_t,32> &seed);
  PublicKey publicKey() const;
  std::array<uint8_t,SECRET_KEY_LENGTH> secretKey() const;
  std::array<uint8_t,32> seed() const;
  std::string toBase58SecretKey() const;
  std::array<uint8_t,SIGNATURE_LENGTH> sign(const std::vector<uint8_t> &msg) const;
  bool verify(const std::vector<uint8_t> &msg,const std::array<uint8_t,SIGNATURE_LENGTH> &sig) const;
private:
  std::array<uint8_t,PUBLIC_KEY_LENGTH> _publicKey{};
  std::array<uint8_t,SECRET_KEY_LENGTH> _secretKey{};
};
} // namespace solana
