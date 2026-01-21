#include "solana/keypair.h"
#include "solana/base58.h"
#include <cstring>
#include <sodium.h>
#include <stdexcept>
namespace solana {
Keypair Keypair::generate(){
  if(sodium_init()<0) throw std::runtime_error("libsodium init failed");
  Keypair kp;
  uint8_t pk[crypto_sign_PUBLICKEYBYTES],sk[crypto_sign_SECRETKEYBYTES];
  crypto_sign_keypair(pk,sk);
  std::memcpy(kp._publicKey.data(),pk,PUBLIC_KEY_LENGTH);
  std::memcpy(kp._secretKey.data(),sk,SECRET_KEY_LENGTH);
  return kp;
}
Keypair Keypair::fromSecretKey(const std::array<uint8_t,SECRET_KEY_LENGTH> &secretKey){
  if(sodium_init()<0) throw std::runtime_error("libsodium init failed");
  Keypair kp;kp._secretKey=secretKey;
  std::memcpy(kp._publicKey.data(),secretKey.data()+32,PUBLIC_KEY_LENGTH);
  return kp;
}
Keypair Keypair::fromSeed(const std::array<uint8_t,32> &seed){
  if(sodium_init()<0) throw std::runtime_error("libsodium init failed");
  Keypair kp;
  uint8_t pk[crypto_sign_PUBLICKEYBYTES],sk[crypto_sign_SECRETKEYBYTES];
  crypto_sign_seed_keypair(pk,sk,seed.data());
  std::memcpy(kp._publicKey.data(),pk,PUBLIC_KEY_LENGTH);
  std::memcpy(kp._secretKey.data(),sk,SECRET_KEY_LENGTH);
  return kp;
}
PublicKey Keypair::publicKey() const{return PublicKey(_publicKey);}
std::array<uint8_t,SECRET_KEY_LENGTH> Keypair::secretKey() const{return _secretKey;}
std::array<uint8_t,32> Keypair::seed() const{
  std::array<uint8_t,32> s;std::memcpy(s.data(),_secretKey.data(),32);return s;
}
std::array<uint8_t,SIGNATURE_LENGTH> Keypair::sign(const std::vector<uint8_t> &message) const{
  std::array<uint8_t,SIGNATURE_LENGTH> sig{};unsigned long long sigLen=0;
  crypto_sign_detached(sig.data(),&sigLen,message.data(),message.size(),_secretKey.data());
  return sig;
}
bool Keypair::verify(const std::vector<uint8_t> &message,const std::array<uint8_t,SIGNATURE_LENGTH> &signature) const{
  return crypto_sign_verify_detached(signature.data(),message.data(),message.size(),_publicKey.data())==0;
}
std::string Keypair::toBase58SecretKey() const{
  std::vector<unsigned char> vec(_secretKey.begin(),_secretKey.end());return Base58::encode(vec);
}
} // namespace solana
