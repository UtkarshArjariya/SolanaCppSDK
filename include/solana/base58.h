#pragma once
#include <string>
#include <vector>
#include <stdexcept>
namespace solana {
class Base58 {
public:
  static std::string encode(const std::vector<unsigned char> &input);
  static std::vector<unsigned char> decode(const std::string &input);
private:
  static const char *const ALPHABET;
  static const signed char ALPHABET_MAP[128];
};
} // namespace solana
