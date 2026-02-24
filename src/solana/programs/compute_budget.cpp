#include "solana/programs/compute_budget.h"
#include "solana/borsh.h"

namespace solana {
namespace programs {
namespace computeBudget {

PublicKey computeBudgetProgramId() {
    return PublicKey(std::string(COMPUTE_BUDGET_PROGRAM_ID));
}

// setComputeUnitLimit: discriminator = 0x02, then u32 LE units
Instruction setComputeUnitLimit(uint32_t units) {
    std::vector<uint8_t> data;
    data.reserve(5);
    borsh::writeU8(data, 0x02);
    borsh::writeU32(data, units);
    return Instruction(computeBudgetProgramId(), {}, data);
}

// setComputeUnitPrice: discriminator = 0x03, then u64 LE microLamports
Instruction setComputeUnitPrice(uint64_t microLamports) {
    std::vector<uint8_t> data;
    data.reserve(9);
    borsh::writeU8(data, 0x03);
    borsh::writeU64(data, microLamports);
    return Instruction(computeBudgetProgramId(), {}, data);
}

} // namespace computeBudget
} // namespace programs
} // namespace solana
