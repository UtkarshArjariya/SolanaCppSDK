#include "solana/programs/spl_token.h"
#include "solana/borsh.h"
#include "solana/programs/system_program.h"

namespace solana {
namespace programs {
namespace splToken {

// ── ATA derivation ────────────────────────────────────────────────────────────

std::pair<PublicKey, uint8_t> findAssociatedTokenAddress(
    const PublicKey &walletAddress,
    const PublicKey &mintAddress)
{
    // Seeds: [walletAddress bytes, tokenProgram bytes, mintAddress bytes]
    auto walletBytes = walletAddress.toBytes();
    auto tokenProgBytes = tokenProgramId().toBytes();
    auto mintBytes = mintAddress.toBytes();

    std::vector<std::vector<uint8_t>> seeds = {
        walletBytes,
        tokenProgBytes,
        mintBytes
    };

    return PublicKey::findProgramAddressSync(seeds, associatedTokenProgramId());
}

// ── Instruction helpers ───────────────────────────────────────────────────────

Instruction transfer(const PublicKey &source,
                     const PublicKey &dest,
                     const PublicKey &authority,
                     uint64_t amount)
{
    // Discriminator 3, then u64 LE amount
    std::vector<uint8_t> data;
    data.reserve(9);
    borsh::writeU8(data, 3);
    borsh::writeU64(data, amount);

    std::vector<AccountMeta> accounts = {
        {source,    false, true},  // source token account (writable)
        {dest,      false, true},  // dest token account (writable)
        {authority, true,  false}, // owner/delegate (signer, readonly)
    };

    return Instruction(tokenProgramId(), accounts, data);
}

Instruction transferChecked(const PublicKey &source,
                            const PublicKey &mint,
                            const PublicKey &dest,
                            const PublicKey &authority,
                            uint64_t amount,
                            uint8_t decimals)
{
    // Discriminator 12, then u64 LE amount, then u8 decimals
    std::vector<uint8_t> data;
    data.reserve(10);
    borsh::writeU8(data, 12);
    borsh::writeU64(data, amount);
    borsh::writeU8(data, decimals);

    std::vector<AccountMeta> accounts = {
        {source,    false, true},  // source token account (writable)
        {mint,      false, false}, // mint (readonly)
        {dest,      false, true},  // dest token account (writable)
        {authority, true,  false}, // owner/delegate (signer, readonly)
    };

    return Instruction(tokenProgramId(), accounts, data);
}

Instruction mintTo(const PublicKey &mint,
                   const PublicKey &dest,
                   const PublicKey &mintAuthority,
                   uint64_t amount)
{
    // Discriminator 7, then u64 LE amount
    std::vector<uint8_t> data;
    data.reserve(9);
    borsh::writeU8(data, 7);
    borsh::writeU64(data, amount);

    std::vector<AccountMeta> accounts = {
        {mint,          false, true},  // mint (writable — supply changes)
        {dest,          false, true},  // destination token account (writable)
        {mintAuthority, true,  false}, // mint authority (signer)
    };

    return Instruction(tokenProgramId(), accounts, data);
}

Instruction burn(const PublicKey &account,
                 const PublicKey &mint,
                 const PublicKey &authority,
                 uint64_t amount)
{
    // Discriminator 8, then u64 LE amount
    std::vector<uint8_t> data;
    data.reserve(9);
    borsh::writeU8(data, 8);
    borsh::writeU64(data, amount);

    std::vector<AccountMeta> accounts = {
        {account,   false, true},  // token account (writable)
        {mint,      false, true},  // mint (writable — supply decreases)
        {authority, true,  false}, // owner/delegate (signer)
    };

    return Instruction(tokenProgramId(), accounts, data);
}

Instruction closeAccount(const PublicKey &account,
                         const PublicKey &destination,
                         const PublicKey &authority)
{
    // Discriminator 9, no data payload
    std::vector<uint8_t> data;
    data.reserve(1);
    borsh::writeU8(data, 9);

    std::vector<AccountMeta> accounts = {
        {account,     false, true},  // token account to close (writable)
        {destination, false, true},  // rent lamport destination (writable)
        {authority,   true,  false}, // owner (signer)
    };

    return Instruction(tokenProgramId(), accounts, data);
}

Instruction createAssociatedTokenAccount(const PublicKey &payer,
                                         const PublicKey &ataAddress,
                                         const PublicKey &walletAddress,
                                         const PublicKey &mintAddress)
{
    // The Associated Token Program create instruction has no instruction data
    // (the runtime discriminates based on the program being invoked).
    std::vector<uint8_t> data; // empty

    PublicKey systemProgram{std::string(SYSTEM_PROGRAM_ID)};
    PublicKey sysvarRent{std::string(SYSVAR_RENT_ID)};

    std::vector<AccountMeta> accounts = {
        {payer,          true,  true},  // payer (signer, writable)
        {ataAddress,     false, true},  // the ATA address (writable)
        {walletAddress,  false, false}, // wallet owner
        {mintAddress,    false, false}, // token mint
        {systemProgram,  false, false}, // System Program
        {tokenProgramId(), false, false}, // Token Program
        {sysvarRent,     false, false}, // SysvarRent
    };

    return Instruction(associatedTokenProgramId(), accounts, data);
}

} // namespace splToken
} // namespace programs
} // namespace solana
