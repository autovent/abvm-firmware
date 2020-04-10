/*
 Copyright (c) 2014-present PlatformIO <contact@platformio.org>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
**/

#include <stdio.h>
#include <trajectory/trap_percent_time.h>
#include <trajectory/trap_accel_max.h>

#include <unity.h>
Trap_PercentTime planner({.33, 0});
// Trap_FixedAccelVel planner({.4, .4, 10});
#include <fstream>
#include <iostream>
// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void test_trajectory_planner_run(void) {
  TEST_ASSERT(planner.is_idle());

  std::ofstream log;
  log.open("/Users/cwoodall/out.csv");

  planner.set_next({80, 800});

  float ts = 10;
  float time = 0;
  float pos = 0;
  log << "time,pos,vel,vel_max" << std::endl;
  for (int i = 0; i < 100; i++) {
    pos = planner.run(pos);
    time += ts;
    log << time << "," << pos << "," << planner.v_last << "," << planner.v_max
        << std::endl;
  }

  planner.force_next({-100, 900});
  for (int i = 0; i < 100; i++) {
    pos = planner.run(pos);
    time += ts;
    log << time << "," << pos << "," << planner.v_last << "," << planner.v_max
        << std::endl;
  }

  // planner.force_next({20, 800});
  // for (int i = 0; i < 100; i++) {
  //     pos = planner.run(pos);
  //     time += ts;
  //     log << time << "," << pos << "," << planner.v_last << "," <<
  //     planner.v_max <<std::endl;

  // }

  log.close();
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_trajectory_planner_run);
  UNITY_END();

  return 0;
}