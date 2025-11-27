#pragma once
#include "solana/instruction.h"
#include "solana/publicKey.h"
#include <cstdint>
#include <string>
#include <vector>
namespace solana {
struct MessageHeader { uint8_t numRequiredSignatures=0; uint8_t numReadonlySignedAccounts=0; uint8_t numReadonlyUnsignedAccounts=0; };
struct CompiledInstruction { uint8_t programIdIndex; std::vector<uint8_t> accountIndices; std::vector<uint8_t> data; };
class Message {
public:
  static Message compile(const std::vector<Instruction> &instructions,const PublicKey &feePayer,const std::string &recentBlockhash);
  std::vector<uint8_t> serialize() const;
  const std::vector<PublicKey> &accountKeys() const{return _accountKeys;}
  uint8_t numRequiredSignatures() const{return _header.numRequiredSignatures;}
private:
  MessageHeader _header; std::vector<PublicKey> _accountKeys; std::string _recentBlockhash; std::vector<CompiledInstruction> _instructions;
};
} // namespace solana
