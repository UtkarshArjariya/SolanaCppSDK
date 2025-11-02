#pragma once
#include "solana/publicKey.h"
#include <cstdint>
#include <vector>
namespace solana {
struct AccountMeta {
  PublicKey pubkey; bool isSigner; bool isWritable;
  AccountMeta(const PublicKey &pk,bool s,bool w):pubkey(pk),isSigner(s),isWritable(w){}
};
struct Instruction {
  PublicKey programId; std::vector<AccountMeta> accounts; std::vector<uint8_t> data;
  Instruction(const PublicKey &pid,const std::vector<AccountMeta> &a,const std::vector<uint8_t> &d):programId(pid),accounts(a),data(d){}
};
} // namespace solana
