#pragma once
#include <array>
#include <cstdint>
#include <string>
namespace solana {
class Bip39 {
public:
  static std::array<uint8_t,64> mnemonicToSeed(const std::string &mnemonic,const std::string &passphrase="");
  static std::array<uint8_t,32> mnemonicToKeypairSeed(const std::string &mnemonic,const std::string &passphrase="");
};
} // namespace solana
