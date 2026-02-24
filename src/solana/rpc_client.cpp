#include "solana/rpc_client.h"
#include "solana/base58.h"
#include "solana/json.h"
#include <cerrno>
#include <chrono>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <thread>

// POSIX socket headers
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

// OpenSSL for TLS (compile with -lssl -lcrypto)
#ifdef SOLANA_SDK_NO_TLS
// TLS disabled at build time
#else
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

namespace solana {

// ── Constructors ────────────────────────────────────────────────────────────

RpcClient::RpcClient(const std::string &endpoint)
    : _endpoint(endpoint), _port(80), _tls(false) {
  parseEndpoint();
}

RpcClient::RpcClient(const std::string &endpoint, const ProxyConfig &proxy)
    : _endpoint(endpoint), _port(80), _tls(false), _proxy(proxy) {
  parseEndpoint();
}

void RpcClient::setProxy(const ProxyConfig &proxy) { _proxy = proxy; }

// ── Endpoint parsing ────────────────────────────────────────────────────────

void RpcClient::parseEndpoint() {
  std::string url = _endpoint;
  if (url.substr(0, 8) == "https://") {
    _tls = true;
    _port = 443;
    url = url.substr(8);
  } else if (url.substr(0, 7) == "http://") {
    url = url.substr(7);
  }
  auto slash = url.find('/');
  std::string hostPort =
      (slash == std::string::npos) ? url : url.substr(0, slash);
  _path = (slash == std::string::npos) ? "/" : url.substr(slash);

  auto colon = hostPort.rfind(':');
  if (colon != std::string::npos) {
    _host = hostPort.substr(0, colon);
    _port = std::stoi(hostPort.substr(colon + 1));
  } else {
    _host = hostPort;
  }
}

// ── Low-level HTTP POST (plain TCP) ─────────────────────────────────────────

std::string RpcClient::httpPost(const std::string &host, int port,
                                const std::string &path,
                                const std::string &body) {
  struct addrinfo hints{}, *res = nullptr;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;
  std::string portStr = std::to_string(port);
  if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0)
    throw std::runtime_error("DNS resolution failed for: " + host);

  int fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd < 0) {
    freeaddrinfo(res);
    throw std::runtime_error("socket() failed");
  }

  if (::connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
    ::close(fd);
    freeaddrinfo(res);
    throw std::runtime_error("connect() failed to " + host + ":" + portStr);
  }
  freeaddrinfo(res);

  // Build HTTP/1.1 request
  std::ostringstream req;
  req << "POST " << path << " HTTP/1.1\r\n"
      << "Host: " << host << "\r\n"
      << "Content-Type: application/json\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Connection: close\r\n"
      << "\r\n"
      << body;
  std::string reqStr = req.str();

  const char *ptr = reqStr.c_str();
  size_t rem = reqStr.size();
  while (rem > 0) {
    ssize_t sent = ::send(fd, ptr, rem, 0);
    if (sent <= 0) {
      ::close(fd);
      throw std::runtime_error("send() failed");
    }
    ptr += sent;
    rem -= sent;
  }

  std::string response;
  char buf[4096];
  ssize_t n;
  while ((n = ::recv(fd, buf, sizeof(buf), 0)) > 0)
    response.append(buf, n);
  ::close(fd);

  auto sep = response.find("\r\n\r\n");
  if (sep == std::string::npos)
    throw std::runtime_error("Malformed HTTP response");
  return response.substr(sep + 4);
}

// ── HTTPS POST (OpenSSL) ────────────────────────────────────────────────────

std::string RpcClient::httpsPost(const std::string &host, int port,
                                 const std::string &path,
                                 const std::string &body) {
#ifdef SOLANA_SDK_NO_TLS
  throw std::runtime_error(
      "TLS support disabled at build time (SOLANA_SDK_NO_TLS). "
      "Use an HTTP proxy or rebuild with OpenSSL.");
#else
  // Initialize OpenSSL
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();

  const SSL_METHOD *method = TLS_client_method();
  SSL_CTX *ctx = SSL_CTX_new(method);
  if (!ctx)
    throw std::runtime_error("SSL_CTX_new failed");

  // Load system CA certificates
  SSL_CTX_set_default_verify_paths(ctx);

  // Resolve and connect TCP
  struct addrinfo hints{}, *res = nullptr;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;
  std::string portStr = std::to_string(port);
  if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0) {
    SSL_CTX_free(ctx);
    throw std::runtime_error("DNS resolution failed for: " + host);
  }

  int fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd < 0) {
    freeaddrinfo(res);
    SSL_CTX_free(ctx);
    throw std::runtime_error("socket() failed");
  }

  if (::connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
    ::close(fd);
    freeaddrinfo(res);
    SSL_CTX_free(ctx);
    throw std::runtime_error("connect() failed to " + host + ":" + portStr);
  }
  freeaddrinfo(res);

  // SSL handshake
  SSL *ssl = SSL_new(ctx);
  SSL_set_fd(ssl, fd);
  SSL_set_tlsext_host_name(ssl, host.c_str()); // SNI
  if (SSL_connect(ssl) <= 0) {
    SSL_free(ssl);
    ::close(fd);
    SSL_CTX_free(ctx);
    throw std::runtime_error("SSL handshake failed with " + host);
  }

  // Build HTTP request
  std::ostringstream req;
  req << "POST " << path << " HTTP/1.1\r\n"
      << "Host: " << host << "\r\n"
      << "Content-Type: application/json\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Connection: close\r\n"
      << "\r\n"
      << body;
  std::string reqStr = req.str();

  // Send over SSL
  int written = SSL_write(ssl, reqStr.c_str(), reqStr.size());
  if (written <= 0) {
    SSL_free(ssl);
    ::close(fd);
    SSL_CTX_free(ctx);
    throw std::runtime_error("SSL_write failed");
  }

  // Receive over SSL
  std::string response;
  char buf[4096];
  int bytesRead;
  while ((bytesRead = SSL_read(ssl, buf, sizeof(buf))) > 0)
    response.append(buf, bytesRead);

  SSL_shutdown(ssl);
  SSL_free(ssl);
  ::close(fd);
  SSL_CTX_free(ctx);

  // Strip HTTP headers
  auto sep = response.find("\r\n\r\n");
  if (sep == std::string::npos)
    throw std::runtime_error("Malformed HTTPS response");
  return response.substr(sep + 4);
#endif
}

// ── RPC POST dispatcher ─────────────────────────────────────────────────────

std::string RpcClient::rpcPost(const std::string &body) {
    if (_proxy.enabled)
        return httpPost(_proxy.host, _proxy.port, _path, body);
    if (_tls)
        return httpsPost(_host, _port, _path, body);
    return httpPost(_host, _port, _path, body);
}

// ── Internal helpers ──────────────────────────────────────────────────────────
// Uses the bundled solana::json parser — handles nested objects, null checks,
// and arrays robustly without string-search fragility.

/// Base64-decode helper for account data returned by RPC
static std::vector<uint8_t> base64DecodeRpc(const std::string &in) {
    static const int8_t T[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    };
    std::vector<uint8_t> out;
    out.reserve((in.size() * 3) / 4);
    int val = 0, valb = -8;
    for (uint8_t c : in) {
        int8_t t = T[c];
        if (t == -1) break;
        val = (val << 6) + t;
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

// Parse an AccountInfo object from a JSON value node.
// node should be the "value" field from getAccountInfo / getMultipleAccounts.
static AccountInfo parseAccountInfoNode(const json::JsonValue &node) {
    AccountInfo info;
    if (node.isNull()) return info;
    info.exists     = true;
    info.lamports   = node["lamports"].asUint();
    info.owner      = node["owner"].asString();
    info.executable = node["executable"].asBool();
    info.rentEpoch  = node["rentEpoch"].asUint();
    // data field is ["<base64 string>", "base64"]
    auto dataArr = node["data"];
    if (dataArr.isArray() && dataArr.arraySize() >= 1)
        info.data = base64DecodeRpc(dataArr[size_t(0)].asString());
    return info;
}

// ── Public RPC API ────────────────────────────────────────────────────────────

std::string RpcClient::getLatestBlockhash() {
    std::string body =
        R"({"jsonrpc":"2.0","id":1,"method":"getLatestBlockhash","params":[{"commitment":"finalized"}]})";
    std::string resp = rpcPost(body);
    auto root = json::JsonValue::parse(resp);
    auto bh = root["result"]["value"]["blockhash"].asString();
    if (bh.empty()) {
        auto errMsg = root["error"]["message"].asString();
        throw std::runtime_error("getLatestBlockhash failed: " +
                                 (errMsg.empty() ? resp : errMsg));
    }
    return bh;
}

std::string RpcClient::sendTransaction(const std::string &base64Tx) {
    std::string body =
        R"({"jsonrpc":"2.0","id":1,"method":"sendTransaction","params":[")" +
        base64Tx +
        R"(", {"encoding":"base64","preflightCommitment":"processed"}]})";
    std::string resp = rpcPost(body);
    auto root = json::JsonValue::parse(resp);
    auto result = root["result"];
    if (result.isString()) return result.asString();
    auto errMsg = root["error"]["message"].asString();
    throw std::runtime_error("sendTransaction failed: " +
                             (errMsg.empty() ? resp : errMsg));
}

std::string RpcClient::simulateTransaction(const std::string &base64Tx,
                                           bool replaceRecentBlockhash) {
    std::string flag = replaceRecentBlockhash ? "true" : "false";
    std::string body =
        R"({"jsonrpc":"2.0","id":1,"method":"simulateTransaction","params":[")" +
        base64Tx +
        R"(", {"encoding":"base64","replaceRecentBlockhash":)" + flag +
        R"(,"commitment":"processed"}]})";
    return rpcPost(body);
}

uint64_t RpcClient::getBalance(const PublicKey &pubkey) {
    std::string body =
        R"({"jsonrpc":"2.0","id":1,"method":"getBalance","params":[")" +
        pubkey.toBase58() + R"(", {"commitment":"finalized"}]})";
    std::string resp = rpcPost(body);
    auto root = json::JsonValue::parse(resp);
    return root["result"]["value"].asUint();
}

AccountInfo RpcClient::getAccountInfo(const PublicKey &pubkey) {
    std::string body =
        R"({"jsonrpc":"2.0","id":1,"method":"getAccountInfo","params":[")" +
        pubkey.toBase58() +
        R"(", {"encoding":"base64","commitment":"finalized"}]})";
    std::string resp = rpcPost(body);
    auto root = json::JsonValue::parse(resp);
    return parseAccountInfoNode(root["result"]["value"]);
}

std::vector<AccountInfo> RpcClient::getMultipleAccountsInfo(
    const std::vector<PublicKey> &pubkeys)
{
    std::ostringstream keys;
    keys << "[";
    for (size_t i = 0; i < pubkeys.size(); ++i) {
        if (i > 0) keys << ",";
        keys << "\"" << pubkeys[i].toBase58() << "\"";
    }
    keys << "]";
    std::string body =
        R"({"jsonrpc":"2.0","id":1,"method":"getMultipleAccounts","params":[)" +
        keys.str() +
        R"(, {"encoding":"base64","commitment":"finalized"}]})";
    std::string resp = rpcPost(body);
    auto root = json::JsonValue::parse(resp);
    auto valueArr = root["result"]["value"];
    std::vector<AccountInfo> result;
    if (!valueArr.isArray()) return result;
    size_t n = valueArr.arraySize();
    result.reserve(n);
    for (size_t i = 0; i < n; ++i)
        result.push_back(parseAccountInfoNode(valueArr[i]));
    return result;
}

bool RpcClient::getAddressLookupTable(const PublicKey &altKey,
                                      AddressLookupTableAccount &out) {
    auto info = getAccountInfo(altKey);
    if (!info.exists || info.data.empty()) return false;
    return AddressLookupTableAccount::parse(altKey, info.data, out);
}

std::string RpcClient::getProgramAccounts(const PublicKey &programId,
                                          const std::string &commitment) {
    std::string body =
        R"({"jsonrpc":"2.0","id":1,"method":"getProgramAccounts","params":[")" +
        programId.toBase58() +
        R"(", {"encoding":"base64","commitment":")" + commitment + R"("}]})";
    return rpcPost(body);
}

std::string RpcClient::requestAirdrop(const PublicKey &pubkey,
                                      uint64_t lamports) {
    std::string body =
        R"({"jsonrpc":"2.0","id":1,"method":"requestAirdrop","params":[")" +
        pubkey.toBase58() + R"(", )" + std::to_string(lamports) + R"(]})";
    std::string resp = rpcPost(body);
    auto root = json::JsonValue::parse(resp);
    auto result = root["result"];
    if (result.isString()) return result.asString();
    auto errMsg = root["error"]["message"].asString();
    throw std::runtime_error("requestAirdrop failed: " +
                             (errMsg.empty() ? resp : errMsg));
}

bool RpcClient::confirmTransaction(const std::string &signature,
                                   uint32_t timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - start)
                           .count();
        if (static_cast<uint32_t>(elapsed) >= timeoutMs) return false;
        try {
            std::string resp = getSignatureStatuses({signature});
            auto root = json::JsonValue::parse(resp);
            auto status = root["result"]["value"][size_t(0)];
            if (!status.isNull()) {
                auto cs = status["confirmationStatus"].asString();
                if (cs == "finalized" || cs == "confirmed" || cs == "processed")
                    return true;
            }
        } catch (...) {}
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}

std::string RpcClient::getSignatureStatuses(
    const std::vector<std::string> &signatures)
{
    std::ostringstream sigs;
    sigs << "[";
    for (size_t i = 0; i < signatures.size(); ++i) {
        if (i > 0) sigs << ",";
        sigs << "\"" << signatures[i] << "\"";
    }
    sigs << "]";
    std::string body =
        R"({"jsonrpc":"2.0","id":1,"method":"getSignatureStatuses","params":[)" +
        sigs.str() + R"(, {"searchTransactionHistory":true}]})";
    return rpcPost(body);
}

uint64_t RpcClient::getSlot() {
    std::string body =
        R"({"jsonrpc":"2.0","id":1,"method":"getSlot","params":[{"commitment":"finalized"}]})";
    std::string resp = rpcPost(body);
    auto root = json::JsonValue::parse(resp);
    return root["result"].asUint();
}

} // namespace solana
