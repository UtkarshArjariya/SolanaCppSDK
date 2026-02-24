#pragma once
#include "solana/address_lookup_table.h"
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
 * Account data returned by getAccountInfo / getMultipleAccountsInfo.
 */
struct AccountInfo {
    uint64_t lamports = 0;
    std::string owner;       ///< Base58 program owner
    std::vector<uint8_t> data; ///< Decoded account data (from base64 field)
    bool executable = false;
    uint64_t rentEpoch = 0;
    bool exists = false;     ///< false if the account was not found (null result)
};

/**
 * Solana JSON-RPC client.
 *
 * Uses the bundled solana::json parser — robust against nested objects,
 * null fields, and large RPC responses without relying on heavy heap trees.
 *
 * Supports both HTTP and HTTPS (via OpenSSL) connections.
 * For embedded/microcontroller use without TLS, connect through
 * an HTTP proxy or use an HTTP-only endpoint.
 */
class RpcClient {
public:
    explicit RpcClient(const std::string &endpoint);
    RpcClient(const std::string &endpoint, const ProxyConfig &proxy);

    // ── Core RPC Methods ─────────────────────────────────────────────────────

    /// Get the latest finalized blockhash for transaction signing.
    std::string getLatestBlockhash();

    /// Send a signed, base64-encoded transaction. Returns tx signature.
    std::string sendTransaction(const std::string &base64Tx);

    /// Simulate a transaction without sending it.
    /// Returns a SimulateTransactionResponse JSON string.
    /// Set replaceRecentBlockhash=true to use the latest blockhash automatically.
    std::string simulateTransaction(const std::string &base64Tx,
                                    bool replaceRecentBlockhash = false);

    /// Get the balance (in lamports) of an account.
    uint64_t getBalance(const PublicKey &pubkey);

    /// Get parsed account info.
    AccountInfo getAccountInfo(const PublicKey &pubkey);

    /// Get multiple account infos in a single RPC call (max 100 per call).
    std::vector<AccountInfo> getMultipleAccountsInfo(
        const std::vector<PublicKey> &pubkeys);

    /// Fetch and parse an Address Lookup Table account.
    /// Returns false if not found or not a valid ALT.
    bool getAddressLookupTable(const PublicKey &altKey,
                               AddressLookupTableAccount &out);

    /// Get all accounts owned by a program.
    /// Returns raw JSON (can be large — use carefully on MCUs).
    std::string getProgramAccounts(const PublicKey &programId,
                                   const std::string &commitment = "finalized");

    /// Request an airdrop of `lamports` to `pubkey` (devnet/testnet only).
    std::string requestAirdrop(const PublicKey &pubkey, uint64_t lamports);

    /// Poll for transaction confirmation. Returns true if confirmed within
    /// `timeoutMs` milliseconds.
    bool confirmTransaction(const std::string &signature,
                            uint32_t timeoutMs = 30000);

    /// Get signature statuses. Returns the raw JSON response.
    std::string getSignatureStatuses(const std::vector<std::string> &signatures);

    /// Get the current slot.
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

