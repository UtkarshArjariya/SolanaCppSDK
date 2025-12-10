#include "solana/programs/system_program.h"
namespace solana { namespace programs {
Instruction transfer(const PublicKey &from,const PublicKey &to,uint64_t lamports){
  std::vector<uint8_t> data(12,0);
  data[0]=2;data[1]=0;data[2]=0;data[3]=0;
  for(int i=0;i<8;++i) data[4+i]=static_cast<uint8_t>(lamports>>(8*i));
  PublicKey sysProg(std::string(SYSTEM_PROGRAM_ID));
  return Instruction(sysProg,{AccountMeta(from,true,true),AccountMeta(to,false,true)},data);
}
}} // namespace solana::programs
