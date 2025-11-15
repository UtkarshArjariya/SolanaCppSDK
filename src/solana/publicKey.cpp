#include "solana/publicKey.h"
#include "solana/base58.h"
#include <stdexcept>
#include <cstring>
namespace solana {
const PublicKey PublicKey::Default=PublicKey(std::array<uint8_t,PUBLIC_KEY_LENGTH>{});
PublicKey::PublicKey(const std::string &base58){
  auto decoded=Base58::decode(base58);
  if(decoded.size()!=PUBLIC_KEY_LENGTH) throw std::runtime_error("Invalid public key input");
  std::memcpy(_bytes.data(),decoded.data(),PUBLIC_KEY_LENGTH);
}
PublicKey::PublicKey(const std::array<uint8_t,PUBLIC_KEY_LENGTH> &bytes){_bytes=bytes;}
PublicKey::PublicKey(const std::vector<uint8_t> &bytes){
  if(bytes.size()!=PUBLIC_KEY_LENGTH) throw std::runtime_error("Invalid public key length");
  std::memcpy(_bytes.data(),bytes.data(),PUBLIC_KEY_LENGTH);
}
PublicKey::PublicKey(uint64_t value){
  _bytes.fill(0);
  for(int i=0;i<8;i++) _bytes[PUBLIC_KEY_LENGTH-1-i]=static_cast<uint8_t>(value>>(8*i));
}
PublicKey PublicKey::unique(){static uint64_t c=1;return PublicKey(c++);}
bool PublicKey::isOnCurve(const PublicKey &){return false;}
bool PublicKey::equals(const PublicKey &other) const{return _bytes==other._bytes;}
std::string PublicKey::toBase58() const{std::vector<unsigned char> v(_bytes.begin(),_bytes.end());return Base58::encode(v);}
std::string PublicKey::toJSON() const{return toBase58();}
std::vector<uint8_t> PublicKey::toBytes() const{return{_bytes.begin(),_bytes.end()};}
std::array<uint8_t,PUBLIC_KEY_LENGTH> PublicKey::toBuffer() const{return _bytes;}
std::string PublicKey::toString() const{return toBase58();}
} // namespace solana
