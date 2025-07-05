#include "main.h"
#include "globals.h"

// External globals instance
extern Globals* globals;

/**
 * @brief GPIO EXTI callback
 * @param GPIO_Pin: GPIO pin that triggered the interrupt
 */
extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // Check which pin triggered the interrupt
    if (GPIO_Pin == CC_GPIO0_Pin || GPIO_Pin == CC_GPIO2_Pin || GPIO_Pin == CC_GPIO3_Pin) {
        // Get the Radio instance
        if (globals != nullptr) {
            CC1200* cc1200 = globals->getCC1200();
            if (cc1200 != nullptr) {
                // Handle the interrupt in the Radio class
                // Handle CC1200 GPIO interrupt directly
                // TODO: Add CC1200 GPIO interrupt handling if needed
            }
        }
    }
}
