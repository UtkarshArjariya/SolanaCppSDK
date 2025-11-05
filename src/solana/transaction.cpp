#include "solana/transaction.h"
#include <cstring>
#include <sodium.h>
#include <stdexcept>

// Base64 encoding (RFC 4648)
static std::string base64Encode(const std::vector<uint8_t> &data) {
  static const char *b64 =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve(((data.size() + 2) / 3) * 4);
  for (size_t i = 0; i < data.size(); i += 3) {
    uint32_t val = data[i] << 16;
    if (i + 1 < data.size())
      val |= data[i + 1] << 8;
    if (i + 2 < data.size())
      val |= data[i + 2];
    out.push_back(b64[(val >> 18) & 63]);
    out.push_back(b64[(val >> 12) & 63]);
    out.push_back(i + 1 < data.size() ? b64[(val >> 6) & 63] : '=');
    out.push_back(i + 2 < data.size() ? b64[(val) & 63] : '=');
  }
  return out;
}

namespace solana {

Transaction::Transaction(const Message &message) : _message(message) {
  // Pre-allocate empty signatures (zeroed) for each required signer
  size_t numSigs = message.numRequiredSignatures();
  _signatures.assign(numSigs, std::array<uint8_t, SIGNATURE_LENGTH>{});
}

void Transaction::sign(const std::vector<Keypair> &signers) {
  if (signers.empty())
    throw std::runtime_error("No signers provided");

  // The message bytes are what we sign
  auto msgBytes = _message.serialize();

  // Match each signer to their position in the account keys list
  auto &keys = _message.accountKeys();
  for (auto &kp : signers) {
    PublicKey pk = kp.publicKey();
    for (size_t i = 0; i < keys.size() && i < _signatures.size(); ++i) {
      if (keys[i].equals(pk)) {
        _signatures[i] = kp.sign(msgBytes);
        break;
      }
    }
  }
}

std::vector<uint8_t> Transaction::serialize() const {
  std::vector<uint8_t> buf;

  // compact-u16 signature count
  uint16_t sigCount = static_cast<uint16_t>(_signatures.size());
  uint8_t b = sigCount & 0x7F;
  sigCount >>= 7;
  if (sigCount > 0)
    b |= 0x80;
  buf.push_back(b);
  if (sigCount > 0)
    buf.push_back(static_cast<uint8_t>(sigCount));

  // Each signature: 64 bytes
  for (auto &sig : _signatures)
    buf.insert(buf.end(), sig.begin(), sig.end());

  // Serialized message
  auto msgBytes = _message.serialize();
  buf.insert(buf.end(), msgBytes.begin(), msgBytes.end());

  return buf;
}

std::string Transaction::toBase64() const { return base64Encode(serialize()); }

const std::vector<std::array<uint8_t, SIGNATURE_LENGTH>> &
Transaction::signatures() const {
  return _signatures;
}

} // namespace solana
