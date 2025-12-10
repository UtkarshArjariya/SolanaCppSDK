#include "solana/rpc_client.h"
#include "solana/base58.h"
#include <cerrno>
#include <cstring>
#include <sstream>
#include <stdexcept>

// POSIX socket headers
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

namespace solana {

// ── Constructor / endpoint parsing ──────────────────────────────────────────

RpcClient::RpcClient(const std::string &endpoint)
    : _endpoint(endpoint), _port(80), _tls(false) {
  parseEndpoint();
}

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

// ── Low-level HTTP POST ──────────────────────────────────────────────────────

std::string RpcClient::rpcPost(const std::string &body) {
  if (_tls)
    throw std::runtime_error("TLS not supported in embedded mode. Use an HTTP "
                             "endpoint or a TLS proxy.");

  // Resolve host
  struct addrinfo hints{}, *res = nullptr;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;
  std::string portStr = std::to_string(_port);
  if (getaddrinfo(_host.c_str(), portStr.c_str(), &hints, &res) != 0)
    throw std::runtime_error("DNS resolution failed for: " + _host);

  int fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd < 0) {
    freeaddrinfo(res);
    throw std::runtime_error("socket() failed");
  }

  if (::connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
    ::close(fd);
    freeaddrinfo(res);
    throw std::runtime_error("connect() failed to " + _host + ":" + portStr);
  }
  freeaddrinfo(res);

  // Build HTTP/1.1 request
  std::ostringstream req;
  req << "POST " << _path << " HTTP/1.1\r\n"
      << "Host: " << _host << "\r\n"
      << "Content-Type: application/json\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Connection: close\r\n"
      << "\r\n"
      << body;
  std::string reqStr = req.str();

  // Send
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

  // Receive
  std::string response;
  char buf[4096];
  ssize_t n;
  while ((n = ::recv(fd, buf, sizeof(buf), 0)) > 0)
    response.append(buf, n);
  ::close(fd);

  // Strip HTTP headers (find \r\n\r\n)
  auto sep = response.find("\r\n\r\n");
  if (sep == std::string::npos)
    throw std::runtime_error("Malformed HTTP response");
  return response.substr(sep + 4);
}

// ── JSON extraction helper ──────────────────────────────────────────────────
// Very simple, purpose-built parser; avoids heavy JSON library dependency.

static std::string extractJsonString(const std::string &json,
                                     const std::string &key) {
  auto pos = json.find("\"" + key + "\"");
  if (pos == std::string::npos)
    return "";
  pos = json.find("\"", pos + key.size() + 2);
  if (pos == std::string::npos)
    return "";
  auto end = json.find("\"", pos + 1);
  if (end == std::string::npos)
    return "";
  return json.substr(pos + 1, end - pos - 1);
}

static uint64_t extractJsonNumber(const std::string &json,
                                  const std::string &key) {
  auto pos = json.find("\"" + key + "\"");
  if (pos == std::string::npos)
    return 0;
  pos = json.find(":", pos);
  if (pos == std::string::npos)
    return 0;
  while (pos < json.size() && !isdigit(json[pos]))
    ++pos;
  return std::stoull(json.substr(pos));
}

// ── Public API ──────────────────────────────────────────────────────────────

std::string RpcClient::getLatestBlockhash() {
  std::string body =
      R"({"jsonrpc":"2.0","id":1,"method":"getLatestBlockhash","params":[{"commitment":"finalized"}]})";
  std::string resp = rpcPost(body);
  std::string bh = extractJsonString(resp, "blockhash");
  if (bh.empty())
    throw std::runtime_error("Could not parse blockhash from response:\n" +
                             resp);
  return bh;
}

std::string RpcClient::sendTransaction(const std::string &base64Tx) {
  std::string body =
      R"({"jsonrpc":"2.0","id":1,"method":"sendTransaction","params":[")" +
      base64Tx +
      R"(", {"encoding":"base64","preflightCommitment":"processed"}]})";
  std::string resp = rpcPost(body);
  // Result is the tx signature string
  std::string sig = extractJsonString(resp, "result");
  if (sig.empty()) {
    // Check for error
    std::string err = extractJsonString(resp, "message");
    throw std::runtime_error("sendTransaction failed: " +
                             (err.empty() ? resp : err));
  }
  return sig;
}

uint64_t RpcClient::getBalance(const PublicKey &pubkey) {
  std::string body =
      R"({"jsonrpc":"2.0","id":1,"method":"getBalance","params":[")" +
      pubkey.toBase58() + R"("]})";
  std::string resp = rpcPost(body);
  return extractJsonNumber(resp, "value");
}

std::string RpcClient::getAccountInfo(const PublicKey &pubkey) {
  std::string body =
      R"({"jsonrpc":"2.0","id":1,"method":"getAccountInfo","params":[")" +
      pubkey.toBase58() + R"(", {"encoding":"base64"}]})";
  return rpcPost(body);
}

} // namespace solana
