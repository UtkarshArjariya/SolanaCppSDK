#pragma once
#include "solana/publicKey.h"
#include <cstdint>
#include <string>
#include <vector>

namespace solana {

/**
 * Configuration for connecting through an HTTP proxy.
 * When set, all RPC requests are sent to the proxy which forwards to the
 * actual Solana RPC endpoint. This allows HTTPS endpoints to be used
 * without TLS support in the SDK itself.
 *
 * Example proxy setup (using mitmproxy):
 *   mitmdump --mode reverse:https://api.devnet.solana.com -p 8080
 *
 * Then use: RpcClient rpc("http://localhost:8080");
 */
struct ProxyConfig {
  std::string host;
  int port = 8080;
  bool enabled = false;
};

/**
 * Solana JSON-RPC client.
 *
 * Supports both HTTP and HTTPS (via OpenSSL) connections.
 * For embedded/microcontroller use without OpenSSL, connect through
 * an HTTP proxy or use an HTTP-only endpoint.
 */
class RpcClient {
public:
  explicit RpcClient(const std::string &endpoint);
  RpcClient(const std::string &endpoint, const ProxyConfig &proxy);

  // ── RPC Methods ──────────────────────────────────────────────────────────

  /// Get the latest blockhash for transaction signing
  std::string getLatestBlockhash();

  /// Send a signed, base64-encoded transaction. Returns tx signature.
  std::string sendTransaction(const std::string &base64Tx);

  /// Get the balance (in lamports) of an account
  uint64_t getBalance(const PublicKey &pubkey);

  /// Get raw account info as JSON string
  std::string getAccountInfo(const PublicKey &pubkey);

  /// Request an airdrop of `lamports` to `pubkey` (devnet/testnet only).
  /// Returns the transaction signature.
  std::string requestAirdrop(const PublicKey &pubkey, uint64_t lamports);

  /// Poll for transaction confirmation. Returns true if confirmed within
  /// `timeoutMs` milliseconds, false if timed out.
  bool confirmTransaction(const std::string &signature,
                          uint32_t timeoutMs = 30000);

  /// Get the status of one or more transaction signatures.
  /// Returns the raw JSON response.
  std::string getSignatureStatuses(const std::vector<std::string> &signatures);

  /// Get the current slot
  uint64_t getSlot();

  // ── Configuration ────────────────────────────────────────────────────────

  void setProxy(const ProxyConfig &proxy);

private:
  std::string _endpoint;
  std::string _host;
  std::string _path;
  int _port;
  bool _tls;
  ProxyConfig _proxy;

  void parseEndpoint();
  std::string rpcPost(const std::string &body);
  std::string httpPost(const std::string &host, int port,
                       const std::string &path, const std::string &body);
  std::string httpsPost(const std::string &host, int port,
                        const std::string &path, const std::string &body);
};

} // namespace solana
