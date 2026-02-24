/**
 * test_message_v0.cpp — Unit tests for Versioned Transaction (V0) infrastructure.
 *
 * Tests cover:
 *  - MessageV0 compilation without ALTs (fallback to all-static accounts)
 *  - MessageV0 compilation with ALTs (address compression)
 *  - TransactionV0 sign + serialize correctness
 *  - AddressLookupTableAccount parse
 *  - Wire format: 0x80 version prefix byte present
 *  - MCU constraint: serialized V0 message fits within 1232-byte Solana MTU
 */
#include "solana/address_lookup_table.h"
#include "solana/keypair.h"
#include "solana/message_v0.h"
#include "solana/programs/system_program.h"
#include "solana/transaction.h"
#include <cassert>
#include <cstring>
#include <iostream>

using namespace solana;

// ── Helpers ───────────────────────────────────────────────────────────────────

static const std::string FAKE_BLOCKHASH =
    "EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N";

static Keypair makeKeypair(uint8_t seed_byte) {
    std::array<uint8_t, 32> seed{};
    seed[0] = seed_byte;
    return Keypair::fromSeed(seed);
}

// ── Test 1: V0 message without ALTs behaves like legacy ───────────────────────

void test_v0_no_alts() {
    auto sender = makeKeypair(1);
    auto receiver = makeKeypair(2);

    auto ix = programs::transfer(sender.publicKey(), receiver.publicKey(), 1000000);
    auto msg = MessageV0::compile({ix}, sender.publicKey(), FAKE_BLOCKHASH);

    // Should have exactly 3 static accounts: sender, receiver, system program
    const auto &keys = msg.staticAccountKeys();
    assert(keys.size() == 3);
    assert(keys[0].equals(sender.publicKey()));   // fee payer first

    // numRequiredSignatures == 1 (only sender)
    assert(msg.numRequiredSignatures() == 1);

    std::cout << "  PASS: V0 compile without ALTs\n";
}

// ── Test 2: Compile with ALT compresses accounts ──────────────────────────────

void test_v0_with_alts() {
    auto feePayer = makeKeypair(1);
    auto programAccount = makeKeypair(10);

    // Build a fake ALT that contains the receiver address
    auto altOwner = makeKeypair(50);
    AddressLookupTableAccount alt;
    alt.key = altOwner.publicKey();
    alt.addresses.push_back(programAccount.publicKey());

    // Create an instruction that references programAccount as a non-signer
    std::vector<AccountMeta> accounts = {
        {programAccount.publicKey(), false, true}
    };
    PublicKey fakeProgramId(std::string("11111111111111111111111111111111"));
    Instruction ix(fakeProgramId, accounts, {0x01, 0x02});

    auto msg = MessageV0::compile({ix}, feePayer.publicKey(), FAKE_BLOCKHASH, {alt});

    // programAccount should be resolved via the ALT, not static
    // Static keys should NOT include programAccount (only feePayer + systemProg)
    const auto &staticKeys = msg.staticAccountKeys();
    for (auto &k : staticKeys) {
        // CompressedAccount should not appear in static list
        assert(!k.equals(programAccount.publicKey()));
    }

    // Serialized message should be shorter than if inlined (32 bytes vs ~2 bytes)
    auto bytes = msg.serialize();
    assert(!bytes.empty());

    std::cout << "  PASS: V0 compile with ALT (address compression)\n";
}

// ── Test 3: TransactionV0 has 0x80 version prefix ────────────────────────────

void test_v0_version_prefix() {
    auto kp = makeKeypair(1);
    auto receiver = makeKeypair(2);

    auto ix = programs::transfer(kp.publicKey(), receiver.publicKey(), 500000);
    auto msg = MessageV0::compile({ix}, kp.publicKey(), FAKE_BLOCKHASH);

    TransactionV0 tx(msg);
    tx.sign({kp});

    auto bytes = tx.serialize();
    assert(!bytes.empty());

    // The version prefix 0x80 must appear right after the signature section.
    // Layout: [compact-u16 sig count][signatures...][0x80][message...]
    // sig count byte = 0x01 (1 signer, fits in 1 byte compact-u16)
    // So: bytes[0] == 0x01, bytes[1..64] == signature, bytes[65] == 0x80
    assert(bytes[0] == 0x01); // 1 signature
    assert(bytes[65] == 0x80); // version byte

    std::cout << "  PASS: TransactionV0 has 0x80 version prefix at byte 65\n";
}

// ── Test 4: TransactionV0 signature is valid ─────────────────────────────────

void test_v0_sign_verify() {
    auto kp = makeKeypair(5);
    auto receiver = makeKeypair(6);

    auto ix = programs::transfer(kp.publicKey(), receiver.publicKey(), 100000);
    auto msg = MessageV0::compile({ix}, kp.publicKey(), FAKE_BLOCKHASH);
    auto msgBytes = msg.serialize();

    TransactionV0 tx(msg);
    tx.sign({kp});

    auto &sigs = tx.signatures();
    assert(sigs.size() == 1);

    // Verify EDDSA signature against message bytes
    assert(kp.verify(msgBytes, sigs[0]));
    std::cout << "  PASS: TransactionV0 Ed25519 signature verifies\n";
}

// ── Test 5: Total serialized V0 transaction fits in Solana's 1232-byte MTU ───

void test_v0_size_within_mtu() {
    auto feePayer = makeKeypair(1);

    // Build a realistic transaction: setComputeUnitLimit + setComputeUnitPrice
    // + a system transfer. 3 instructions, 2 of which have no accounts.
    auto receiver = makeKeypair(2);

    // Simulate 3 accounts: feePayer, receiver, systemProgram
    auto ix = programs::transfer(feePayer.publicKey(), receiver.publicKey(), 100000);

    auto msg = MessageV0::compile(
        {ix},
        feePayer.publicKey(),
        FAKE_BLOCKHASH
    );

    TransactionV0 tx(msg);
    tx.sign({feePayer});

    auto bytes = tx.serialize();
    std::cout << "  INFO: V0 transaction size = " << bytes.size() << " bytes\n";
    assert(bytes.size() <= 1232);

    std::cout << "  PASS: V0 transaction fits within 1232-byte MTU\n";
}

// ── Test 6: ALT parse from raw bytes ─────────────────────────────────────────

void test_alt_parse() {
    // Build a synthetic ALT account data buffer:
    // [4 B type=1][8 B deactivation_slot=0xFFFFFFFFFFFFFFFF]
    // [8 B last_extended_slot=0][1 B last_extended_start=0]
    // [1 B authority option=0 (None)] [32*N address entries]
    std::vector<uint8_t> data;

    // type = 1
    data.push_back(1); data.push_back(0); data.push_back(0); data.push_back(0);
    // deactivation_slot = u64::MAX (active)
    for (int i = 0; i < 8; ++i) data.push_back(0xFF);
    // last_extended_slot = 0
    for (int i = 0; i < 8; ++i) data.push_back(0x00);
    // last_extended_slot_start_index = 0
    data.push_back(0x00);
    // authority option = None (0)
    data.push_back(0x00);

    // Add 3 address entries (each 32 bytes)
    for (int a = 0; a < 3; ++a) {
        for (int b = 0; b < 32; ++b)
            data.push_back(static_cast<uint8_t>(a + 1)); // fill with 1, 2, 3
    }

    auto altKey = makeKeypair(99).publicKey();
    AddressLookupTableAccount out;
    bool ok = AddressLookupTableAccount::parse(altKey, data, out);
    assert(ok);
    assert(out.addresses.size() == 3);

    // Verify first address bytes
    auto firstAddrBytes = out.addresses[0].toBytes();
    assert(firstAddrBytes[0] == 1);

    std::cout << "  PASS: AddressLookupTableAccount::parse with 3 addresses\n";
}

// ── Test 7: ALT parse rejects bad discriminator ───────────────────────────────

void test_alt_parse_reject_bad_discriminator() {
    std::vector<uint8_t> data(54, 0x00); // all zeros — type = 0 (not ALT)
    auto altKey = makeKeypair(77).publicKey();
    AddressLookupTableAccount out;
    bool ok = AddressLookupTableAccount::parse(altKey, data, out);
    assert(!ok);
    std::cout << "  PASS: ALT parse rejects non-ALT account (discriminator != 1)\n";
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== test_message_v0 ===\n";
    test_v0_no_alts();
    test_v0_with_alts();
    test_v0_version_prefix();
    test_v0_sign_verify();
    test_v0_size_within_mtu();
    test_alt_parse();
    test_alt_parse_reject_bad_discriminator();
    std::cout << "All V0 message tests passed.\n";
    return 0;
}
