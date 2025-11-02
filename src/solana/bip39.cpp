#include "solana/bip39.h"
#include <cstring>
#include <sodium.h>
#include <stdexcept>
#include <vector>
#include <algorithm>
namespace solana {
static void pbkdf2_hmac_sha512(const uint8_t *password,size_t plen,const uint8_t *salt,size_t slen,uint32_t iterations,uint8_t *out,size_t outlen){
  if(sodium_init()<0) throw std::runtime_error("libsodium init failed");
  const size_t hlen=crypto_auth_hmacsha512_BYTES;
  uint32_t blockCount=static_cast<uint32_t>((outlen+hlen-1)/hlen);
  for(uint32_t block=1;block<=blockCount;++block){
    std::vector<uint8_t> saltBlock(slen+4);
    std::memcpy(saltBlock.data(),salt,slen);
    saltBlock[slen]=(block>>24)&0xFF;saltBlock[slen+1]=(block>>16)&0xFF;
    saltBlock[slen+2]=(block>>8)&0xFF;saltBlock[slen+3]=(block)&0xFF;
    uint8_t U[64],T[64];
    crypto_auth_hmacsha512_state state;
    crypto_auth_hmacsha512_init(&state,password,plen);
    crypto_auth_hmacsha512_update(&state,saltBlock.data(),saltBlock.size());
    crypto_auth_hmacsha512_final(&state,U);
    std::memcpy(T,U,hlen);
    for(uint32_t i=1;i<iterations;++i){
      crypto_auth_hmacsha512_init(&state,password,plen);
      crypto_auth_hmacsha512_update(&state,U,hlen);
      crypto_auth_hmacsha512_final(&state,U);
      for(size_t j=0;j<hlen;++j) T[j]^=U[j];
    }
    size_t blockStart=(block-1)*hlen;size_t toCopy=std::min(hlen,outlen-blockStart);
    std::memcpy(out+blockStart,T,toCopy);
  }
}
std::array<uint8_t,64> Bip39::mnemonicToSeed(const std::string &mnemonic,const std::string &passphrase){
  std::string saltStr="mnemonic"+passphrase;
  std::array<uint8_t,64> seed{};
  pbkdf2_hmac_sha512(reinterpret_cast<const uint8_t*>(mnemonic.data()),mnemonic.size(),
    reinterpret_cast<const uint8_t*>(saltStr.data()),saltStr.size(),2048,seed.data(),64);
  return seed;
}
std::array<uint8_t,32> Bip39::mnemonicToKeypairSeed(const std::string &mnemonic,const std::string &passphrase){
  auto full=mnemonicToSeed(mnemonic,passphrase);
  std::array<uint8_t,32> s;std::memcpy(s.data(),full.data(),32);return s;
}
} // namespace solana
