#include "drivers/pin.h"

bool Pin::read() { return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET; }

void Pin::write(bool val) {
  HAL_GPIO_WritePin(port, pin, val ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
