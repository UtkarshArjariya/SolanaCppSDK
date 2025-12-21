#include "solana/publicKey.h"
#include <cassert>
#include <iostream>
using namespace solana;
void test_base58(){std::string k="11111111111111111111111111111111";PublicKey pk(k);assert(pk.toBase58()==k);std::cout<<"  PASS: fromBase58 roundtrip\n";}
void test_eq(){std::string a="11111111111111111111111111111111";PublicKey x(a),y(a);assert(x.equals(y));std::cout<<"  PASS: equality\n";}
void test_default(){assert(PublicKey::Default.toBase58()=="11111111111111111111111111111111");std::cout<<"  PASS: default zero key\n";}
int main(){std::cout<<"=== test_publicKey ===\n";test_base58();test_eq();test_default();std::cout<<"All publicKey tests passed.\n";}
