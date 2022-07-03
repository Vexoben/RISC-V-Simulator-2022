#include <iostream>
#include "scanner.hpp"
#include "include.hpp"
#include "core_2bit_2.hpp"

void update() {
   pc_in = pc_out;
   for (int i = 0; i < reg_num; ++i) {
      reg_in[i] = reg_out[i];
      reg_in[i].write_protect = reg_out[i].write_protect = NO;
   }
   myIQ.update();
   myRS.update();
   myROB.update();
   mySLB.update();
   myALU.update();
   BP.update();
}

int main() {
//	freopen("array_test1.data", "r", stdin);
   input();
   int clk = 0;
   while (1) {
      ++clk;
      // puts("run update");
      update();
      // puts("run fetch");
      fetch();
      // puts("run issue");
      issue();
      // puts("run reservation");
      run_reservation();
      // puts("run slbuffer");
      run_slbuffer();
      // puts("run ex");
      run_ex();
      // puts("run rob");
      if (run_rob()) break;
      reg_out[0].Qj = -1;
      reg_out[0].value = 0;
      // debug(pc_in);
      // debug(pc_out);
      // debug(pc_pred);
      // debug(commit_times);
      // for (int i = 0; i < reg_num; ++i) {
      //    std::cout << reg_out[i].value << ' ';
      // }
      // puts("");
      // std::cout << "--------------------------------" << std::endl;
      // if (clk == 5000) break;
   }
   update();
   std::cout << total_predict << '|' << accurate_predict << '|' << 1.0 * accurate_predict / total_predict << '|' << clk << '|' << std::endl;
   // std::cout << "total_predict: " << total_predict << std::endl;
   // std::cout << "accurate_predict: " << accurate_predict << std::endl;
   // std::cout << "accurate_radio: " << 1.0 * accurate_predict / total_predict << std::endl;
   // std::cout << "total clock: " << clk << std::endl;
   std::cout << ((unsigned int) reg_in[10].value & 255u) << std::endl;
   return 0;
}
