#include "solana/message.h"
#include "solana/base58.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace solana {
Message Message::compile(const std::vector<Instruction> &instructions,const PublicKey &feePayer,const std::string &recentBlockhash){
  Message msg;msg._recentBlockhash=recentBlockhash;
  std::vector<PublicKey> orderedKeys;std::vector<bool> isSigner,isWritable;
  auto findOrAdd=[&](const PublicKey &pk,bool signer,bool writable)->uint8_t{
    for(size_t i=0;i<orderedKeys.size();++i){if(orderedKeys[i].equals(pk)){isSigner[i]=isSigner[i]||signer;isWritable[i]=isWritable[i]||writable;return static_cast<uint8_t>(i);}}
    orderedKeys.push_back(pk);isSigner.push_back(signer);isWritable.push_back(writable);return static_cast<uint8_t>(orderedKeys.size()-1);
  };
  findOrAdd(feePayer,true,true);
  for(auto &instr:instructions){for(auto &am:instr.accounts)findOrAdd(am.pubkey,am.isSigner,am.isWritable);findOrAdd(instr.programId,false,false);}
  uint8_t numSigners=0;for(bool s:isSigner)if(s)numSigners++;
  uint8_t nrs=0;for(size_t i=0;i<orderedKeys.size();++i)if(isSigner[i]&&!isWritable[i])nrs++;
  uint8_t nru=0;for(size_t i=0;i<orderedKeys.size();++i)if(!isSigner[i]&&!isWritable[i])nru++;
  msg._header.numRequiredSignatures=numSigners;msg._header.numReadonlySignedAccounts=nrs;msg._header.numReadonlyUnsignedAccounts=nru;
  msg._accountKeys=orderedKeys;
  for(auto &instr:instructions){
    CompiledInstruction ci;
    for(size_t i=0;i<orderedKeys.size();++i)if(orderedKeys[i].equals(instr.programId)){ci.programIdIndex=static_cast<uint8_t>(i);break;}
    for(auto &am:instr.accounts)for(size_t i=0;i<orderedKeys.size();++i)if(orderedKeys[i].equals(am.pubkey)){ci.accountIndices.push_back(static_cast<uint8_t>(i));break;}
    ci.data=instr.data;msg._instructions.push_back(std::move(ci));
  }
  return msg;
}
static void writeCompactU16(std::vector<uint8_t> &buf,uint16_t v){do{uint8_t b=v&0x7F;v>>=7;if(v>0)b|=0x80;buf.push_back(b);}while(v>0);}
std::vector<uint8_t> Message::serialize() const{
  std::vector<uint8_t> buf;
  buf.push_back(_header.numRequiredSignatures);buf.push_back(_header.numReadonlySignedAccounts);buf.push_back(_header.numReadonlyUnsignedAccounts);
  writeCompactU16(buf,static_cast<uint16_t>(_accountKeys.size()));
  for(auto &pk:_accountKeys){auto b=pk.toBytes();buf.insert(buf.end(),b.begin(),b.end());}
  auto bhBytes=Base58::decode(_recentBlockhash);if(bhBytes.size()!=32)throw std::runtime_error("Invalid blockhash length");
  buf.insert(buf.end(),bhBytes.begin(),bhBytes.end());
  writeCompactU16(buf,static_cast<uint16_t>(_instructions.size()));
  for(auto &ci:_instructions){buf.push_back(ci.programIdIndex);writeCompactU16(buf,static_cast<uint16_t>(ci.accountIndices.size()));buf.insert(buf.end(),ci.accountIndices.begin(),ci.accountIndices.end());writeCompactU16(buf,static_cast<uint16_t>(ci.data.size()));buf.insert(buf.end(),ci.data.begin(),ci.data.end());}
  return buf;
}
} // namespace solana
