#include <iostream>
#include "scanner.hpp"
#include "include.hpp"
#include "core.hpp"

void update() {
   for (int i = 0; i < reg_num; ++i) {
      reg_in[i] = reg_out[i];
   }
   myIQ.update();
   myRS.update();
   myROB.update();
   mySLB.update();
}

int main() {
   input();
   int clk = 0;
   while (1) {
      ++clk;
      update();
      fetch();
      issue();
      run_reservation();
      run_slbuffer();
      run_ex();
      run_rob();
      pc_out += 4;
      pc_pred += 4;
      reg_out[0].value = 0;
   }
   std::cout << ((unsigned int) reg_in[10].value & 255u) << std::endl;
   return 0;
}