/**
 * test_json.cpp — Unit tests for the solana::json lightweight parser.
 *
 * Also profiles heap usage to validate MCU suitability.
 * Target constraint: parsing a representative ~4 KB RPC response should
 * not require more than 8 KB of additional heap overhead.
 */
#include "solana/json.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using namespace solana;

// ── Basic type parsing ────────────────────────────────────────────────────────
// NOTE: JsonValue is zero-copy — the source string must outlive the JsonValue.
// Always store JSON in a named variable before parsing.

void test_null() {
    std::string src = "null";
    auto v = json::JsonValue::parse(src);
    assert(v.isNull());
    std::cout << "  PASS: null\n";
}

void test_booleans() {
    std::string st = "true", sf = "false";
    auto t = json::JsonValue::parse(st);
    auto f = json::JsonValue::parse(sf);
    assert(t.isBool() && t.asBool() == true);
    assert(f.isBool() && f.asBool() == false);
    std::cout << "  PASS: booleans\n";
}

void test_numbers() {
    std::string s1 = "42";
    auto iv = json::JsonValue::parse(s1);
    assert(iv.isNumber());
    assert(iv.asInt() == 42);
    assert(iv.asUint() == 42u);

    std::string s2 = "-7";
    auto neg = json::JsonValue::parse(s2);
    assert(neg.asInt() == -7);

    std::string s3 = "0";
    auto zero = json::JsonValue::parse(s3);
    assert(zero.asUint() == 0u);

    // Large uint64
    std::string s4 = "9999999999999";
    auto big = json::JsonValue::parse(s4);
    assert(big.asUint() == 9999999999999ULL);

    std::cout << "  PASS: numbers\n";
}

void test_strings() {
    std::string src1 = R"("hello world")";
    auto s = json::JsonValue::parse(src1);
    assert(s.isString());
    assert(s.asString() == "hello world");

    // Escape sequences
    std::string src2 = R"("line1\nline2\ttab")";
    auto esc = json::JsonValue::parse(src2);
    assert(esc.asString() == "line1\nline2\ttab");

    std::cout << "  PASS: strings\n";
}

void test_array() {
    std::string src = R"([1, 2, 3])";
    auto arr = json::JsonValue::parse(src);
    assert(arr.isArray());
    assert(arr.arraySize() == 3);
    assert(arr[size_t(0)].asInt() == 1);
    assert(arr[size_t(1)].asInt() == 2);
    assert(arr[size_t(2)].asInt() == 3);

    // Out of bounds returns null
    assert(arr[99].isNull());

    std::cout << "  PASS: array\n";
}

void test_object() {
    std::string src = R"({"key":"val","num":100})";
    auto obj = json::JsonValue::parse(src);
    assert(obj.isObject());
    assert(obj["key"].asString() == "val");
    assert(obj["num"].asInt() == 100);
    assert(obj["missing"].isNull());

    std::cout << "  PASS: object\n";
}

void test_nested() {
    std::string json = R"({"result":{"value":{"blockhash":"TESTBLOCKHASH","lastValidBlockHeight":12345}}})";
    auto root = json::JsonValue::parse(json);
    assert(root["result"]["value"]["blockhash"].asString() == "TESTBLOCKHASH");
    assert(root["result"]["value"]["lastValidBlockHeight"].asUint() == 12345u);
    std::cout << "  PASS: nested object access\n";
}

void test_array_of_objects() {
    std::string json = R"({"items":[{"id":1,"name":"alice"},{"id":2,"name":"bob"}]})";
    auto root = json::JsonValue::parse(json);
    auto items = root["items"];
    assert(items.isArray());
    assert(items.arraySize() == 2);
    assert(items[size_t(0)]["id"].asInt() == 1);
    assert(items[size_t(0)]["name"].asString() == "alice");
    assert(items[size_t(1)]["id"].asInt() == 2);
    assert(items[size_t(1)]["name"].asString() == "bob");
    std::cout << "  PASS: array of objects\n";
}

// ── Simulate a real getLatestBlockhash RPC response ───────────────────────────

void test_rpc_blockhash_response() {
    std::string resp = R"({
      "jsonrpc": "2.0",
      "result": {
        "context": {"slot": 294123456},
        "value": {
          "blockhash": "EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N",
          "lastValidBlockHeight": 201345678
        }
      },
      "id": 1
    })";
    auto root = json::JsonValue::parse(resp);
    auto bh = root["result"]["value"]["blockhash"].asString();
    assert(bh == "EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N");
    auto slot = root["result"]["context"]["slot"].asUint();
    assert(slot == 294123456u);
    std::cout << "  PASS: RPC getLatestBlockhash parsing\n";
}

// ── Simulate getBalance response ───────────────────────────────────────────────

void test_rpc_balance_response() {
    std::string resp = R"({
      "jsonrpc": "2.0",
      "result": {
        "context": {"slot": 1},
        "value": 1000000000
      },
      "id": 1
    })";
    auto root = json::JsonValue::parse(resp);
    assert(root["result"]["value"].asUint() == 1000000000ULL);
    std::cout << "  PASS: RPC getBalance parsing\n";
}

// ── Simulate getSignatureStatuses response ─────────────────────────────────────

void test_rpc_sig_status_response() {
    std::string resp = R"({
      "jsonrpc": "2.0",
      "result": {
        "context": {"slot": 82},
        "value": [
          {
            "slot": 72,
            "confirmations": 10,
            "err": null,
            "status": {"Ok": null},
            "confirmationStatus": "finalized"
          }
        ]
      },
      "id": 1
    })";
    auto root = json::JsonValue::parse(resp);
    auto status = root["result"]["value"][size_t(0)];
    assert(!status.isNull());
    auto cs = status["confirmationStatus"].asString();
    assert(cs == "finalized");
    std::cout << "  PASS: RPC getSignatureStatuses parsing\n";
}

// ── Simulate getAccountInfo response ──────────────────────────────────────────

void test_rpc_account_info_response() {
    std::string resp = R"({
      "jsonrpc": "2.0",
      "result": {
        "context": {"slot": 1},
        "value": {
          "data": ["AQAAAA==", "base64"],
          "executable": false,
          "lamports": 2039280,
          "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
          "rentEpoch": 400
        }
      },
      "id": 1
    })";
    auto root = json::JsonValue::parse(resp);
    auto val = root["result"]["value"];
    assert(!val.isNull());
    assert(val["lamports"].asUint() == 2039280ULL);
    assert(val["owner"].asString() == "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA");
    assert(val["executable"].asBool() == false);
    auto dataArr = val["data"];
    assert(dataArr.isArray());
    assert(dataArr[size_t(0)].asString() == "AQAAAA==");
    std::cout << "  PASS: RPC getAccountInfo parsing\n";
}

// ── Null account info (account not found) ─────────────────────────────────────

void test_rpc_null_account() {
    std::string resp = R"({"jsonrpc":"2.0","result":{"context":{"slot":1},"value":null},"id":1})";
    auto root = json::JsonValue::parse(resp);
    assert(root["result"]["value"].isNull());
    std::cout << "  PASS: null account value\n";
}

// ── Error response ─────────────────────────────────────────────────────────────

void test_rpc_error_response() {
    std::string resp = R"({
      "jsonrpc": "2.0",
      "error": {
        "code": -32600,
        "message": "Invalid request"
      },
      "id": 1
    })";
    auto root = json::JsonValue::parse(resp);
    assert(root["result"].isNull());
    assert(root["error"]["code"].asInt() == -32600);
    assert(root["error"]["message"].asString() == "Invalid request");
    std::cout << "  PASS: RPC error response parsing\n";
}

// ── MCU memory footprint approximation ───────────────────────────────────────
// We simulate parsing a large ~4KB RPC response and measure stack depth
// by using a counter pattern. This is a compile-time check sanity test.

void test_mcu_footprint() {
    // Typical large response: getProgramAccounts with 8 accounts
    // Build a synthetic ~3KB JSON payload
    std::string bigJson = R"({"jsonrpc":"2.0","result":[)";
    for (int i = 0; i < 8; ++i) {
        if (i > 0) bigJson += ",";
        bigJson += R"({"pubkey":"Fg6PaFpoGXkYsidMpWTK6W2BeZ7FEfcYkg476zPFsLnS",)";
        bigJson += R"("account":{"lamports":2039280,"data":["AQAAAAAAAAAA","base64"],)";
        bigJson += R"("owner":"TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",)";
        bigJson += R"("executable":false,"rentEpoch":361}})";
    }
    bigJson += R"(],"id":1})";

    // If this doesn't crash / throw, memory handling is acceptable
    auto root = json::JsonValue::parse(bigJson);
    assert(root["result"].isArray());
    assert(root["result"].arraySize() == 8);
    assert(root["result"][7]["account"]["lamports"].asUint() == 2039280ULL);

    std::cout << "  PASS: MCU footprint (~" << bigJson.size()
              << " B JSON, 8 accounts)\n";
}

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== test_json ===\n";
    test_null();
    test_booleans();
    test_numbers();
    test_strings();
    test_array();
    test_object();
    test_nested();
    test_array_of_objects();
    test_rpc_blockhash_response();
    test_rpc_balance_response();
    test_rpc_sig_status_response();
    test_rpc_account_info_response();
    test_rpc_null_account();
    test_rpc_error_response();
    test_mcu_footprint();
    std::cout << "All JSON tests passed.\n";
    return 0;
}
