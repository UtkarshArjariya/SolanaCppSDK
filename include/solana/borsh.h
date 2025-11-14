#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace solana { namespace borsh {
inline void writeU8(std::vector<uint8_t> &buf,uint8_t v){buf.push_back(v);}
inline void writeU32(std::vector<uint8_t> &buf,uint32_t v){buf.push_back(v&0xFF);buf.push_back((v>>8)&0xFF);buf.push_back((v>>16)&0xFF);buf.push_back((v>>24)&0xFF);}
inline void writeU64(std::vector<uint8_t> &buf,uint64_t v){for(int i=0;i<8;++i)buf.push_back((v>>(8*i))&0xFF);}
inline void writeString(std::vector<uint8_t> &buf,const std::string &s){writeU32(buf,static_cast<uint32_t>(s.size()));buf.insert(buf.end(),s.begin(),s.end());}
inline void writeBytes(std::vector<uint8_t> &buf,const std::vector<uint8_t> &v){buf.insert(buf.end(),v.begin(),v.end());}
}} // namespace solana::borsh
