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

#include <trajectory_planner.h>
#include <unity.h>
#include <stdio.h>
TrajectoryPlanner planner({800, -800});

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

void test_trajectory_planner_run(void) {
    TEST_ASSERT(planner.is_idle());

    planner.set_next({90, (360 * 20.0/10)/(2*3.14159), .8});

    float ts = 10;
    for (int i = 0; i < 100; i++) {
        float pos = planner.run(0);
        printf("%f, %d\n", pos, (uint32_t)planner.get_state());
        set_millis(millis() + ts);
    }
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_trajectory_planner_run);
    UNITY_END();

    return 0;
}