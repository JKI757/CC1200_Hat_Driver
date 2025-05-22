#include "main.h"
#include "globals.h"
#include "Radio.h"

// External globals instance
extern Globals* g_globals;

/**
 * @brief GPIO EXTI callback
 * @param GPIO_Pin: GPIO pin that triggered the interrupt
 */
extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // Check which pin triggered the interrupt
    if (GPIO_Pin == CC_GPIO0_Pin || GPIO_Pin == CC_GPIO2_Pin || GPIO_Pin == CC_GPIO3_Pin) {
        // Get the Radio instance
        if (g_globals != nullptr) {
            Radio* radio = g_globals->getRadio();
            if (radio != nullptr) {
                // Handle the interrupt in the Radio class
                radio->handleGPIOInterrupt(GPIO_Pin);
            }
        }
    }
}
