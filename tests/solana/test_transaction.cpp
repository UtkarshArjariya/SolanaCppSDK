#include "solana/keypair.h"
#include "solana/message.h"
#include "solana/transaction.h"
#include "solana/instruction.h"
#include <cassert>
#include <iostream>
using namespace solana;
void test_serialize(){auto fp=Keypair::generate();std::string fh="11111111111111111111111111111111";PublicKey sp(fh);Instruction ix(sp,{},{0x02,0x00,0x00,0x00});auto msg=Message::compile({ix},fp.publicKey(),fh);Transaction tx(msg);tx.sign({fp});auto b=tx.serialize();assert(!b.empty()&&b.size()>64);std::cout<<"  PASS: serialize\n";}
void test_b64(){auto fp=Keypair::generate();std::string fh="11111111111111111111111111111111";PublicKey sp(fh);Instruction ix(sp,{},{0x01});auto msg=Message::compile({ix},fp.publicKey(),fh);Transaction tx(msg);tx.sign({fp});auto s=tx.toBase64();assert(!s.empty());std::cout<<"  PASS: base64\n";}
int main(){std::cout<<"=== test_transaction ===\n";test_serialize();test_b64();std::cout<<"All transaction tests passed.\n";}
