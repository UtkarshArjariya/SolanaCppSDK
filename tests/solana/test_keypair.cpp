#include "solana/keypair.h"
#include <cassert>
#include <iostream>
#include <vector>
using namespace solana;
void test_generate(){auto kp=Keypair::generate();assert(!kp.publicKey().toBase58().empty());std::cout<<"  PASS: generate\n";}
void test_from_seed(){std::array<uint8_t,32> s{};s[0]=1;s[31]=255;auto k1=Keypair::fromSeed(s);auto k2=Keypair::fromSeed(s);assert(k1.publicKey().equals(k2.publicKey()));std::cout<<"  PASS: fromSeed deterministic\n";}
void test_sign_verify(){auto kp=Keypair::generate();std::vector<uint8_t> m={1,2,3,4,5};auto sig=kp.sign(m);assert(kp.verify(m,sig));m[0]^=0xFF;assert(!kp.verify(m,sig));std::cout<<"  PASS: sign/verify\n";}
void test_from_sk(){auto kp=Keypair::generate();auto kp2=Keypair::fromSecretKey(kp.secretKey());assert(kp.publicKey().equals(kp2.publicKey()));std::cout<<"  PASS: fromSecretKey\n";}
int main(){std::cout<<"=== test_keypair ===\n";test_generate();test_from_seed();test_sign_verify();test_from_sk();std::cout<<"All keypair tests passed.\n";}
