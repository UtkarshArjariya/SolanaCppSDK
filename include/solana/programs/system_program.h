#pragma once
#include "solana/instruction.h"
#include "solana/publicKey.h"
#include <cstdint>
namespace solana { namespace programs {
static const char *const SYSTEM_PROGRAM_ID = "11111111111111111111111111111111";
Instruction transfer(const PublicKey &from,const PublicKey &to,uint64_t lamports);
}} // namespace solana::programs
