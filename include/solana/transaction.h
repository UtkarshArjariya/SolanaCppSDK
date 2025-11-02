#pragma once
#include "solana/keypair.h"
#include "solana/message.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace solana {
class Transaction {
public:
  explicit Transaction(const Message &message);
  void sign(const std::vector<Keypair> &signers);
  std::vector<uint8_t> serialize() const;
  std::string toBase64() const;
  const std::vector<std::array<uint8_t,SIGNATURE_LENGTH>> &signatures() const;
private:
  Message _message;
  std::vector<std::array<uint8_t,SIGNATURE_LENGTH>> _signatures;
};
} // namespace solana
