#pragma once
#include "solana/publicKey.h"
#include <cstdint>
#include <string>
namespace solana {
class RpcClient {
public:
  explicit RpcClient(const std::string &endpoint);
  std::string getLatestBlockhash();
  std::string sendTransaction(const std::string &base64Tx);
  uint64_t getBalance(const PublicKey &pubkey);
  std::string getAccountInfo(const PublicKey &pubkey);
private:
  std::string _endpoint,_host,_path; int _port; bool _tls;
  void parseEndpoint(); std::string rpcPost(const std::string &body);
};
} // namespace solana
