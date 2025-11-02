#include "solana/base58.h"
#include <cassert>
#include <iostream>
#include <vector>
using namespace solana;
void test_roundtrip(){std::vector<unsigned char> d={0x00,0x01,0x02,0xAB,0xCD,0xEF};auto e=Base58::encode(d);assert(Base58::decode(e)==d);std::cout<<"  PASS: roundtrip\n";}
void test_known(){std::vector<unsigned char> z(32,0);assert(Base58::encode(z)=="11111111111111111111111111111111");std::cout<<"  PASS: known zero\n";}
void test_empty(){assert(Base58::encode({})==""&&Base58::decode("").empty());std::cout<<"  PASS: empty\n";}
void test_invalid(){bool t=false;try{Base58::decode("0OIl");}catch(const std::invalid_argument&){t=true;}assert(t);std::cout<<"  PASS: invalid char\n";}
int main(){std::cout<<"=== test_base58 ===\n";test_roundtrip();test_known();test_empty();test_invalid();std::cout<<"All base58 tests passed.\n";}
