/*
 * SPI DMA Callback Functions for CC1200 Integration
 * 
 * This file provides the bridge between STM32 HAL DMA callbacks
 * and the CC1200 driver DMA handling functions.
 */

#include "stm32f4xx_hal.h"
#include "globals.h"

// External reference to global CC1200 instance
extern Globals* globals;

/**
 * @brief SPI1 DMA TX Complete Callback
 * Called when SPI1 DMA transmission is complete
 */
extern "C" void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        // SPI1 TX DMA complete - handled by combined TxRx callback
    }
}

/**
 * @brief SPI1 DMA RX Complete Callback  
 * Called when SPI1 DMA reception is complete
 */
extern "C" void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
        // SPI1 RX DMA complete - handled by combined TxRx callback
    }
}

/**
 * @brief SPI1 DMA TX/RX Complete Callback
 * Called when SPI1 DMA bidirectional transfer is complete
 */
extern "C" void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1 && globals != nullptr) {
        CC1200* cc1200 = globals->getCC1200();
        if (cc1200 != nullptr) {
            cc1200->dmaTransferCompleteCallback();
        }
    }
}

/**
 * @brief SPI1 DMA Error Callback
 * Called when SPI1 DMA transfer encounters an error
 */
extern "C" void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1 && globals != nullptr) {
        CC1200* cc1200 = globals->getCC1200();
        if (cc1200 != nullptr) {
            cc1200->dmaTransferErrorCallback();
        }
    }
}

/**
 * @brief DMA2 Stream0 (SPI1_RX) Complete Callback
 * Called when DMA2 Stream0 transfer is complete
 */
extern "C" void HAL_DMA_XferCpltCallback(DMA_HandleTypeDef *hdma)
{
    // This is called by the HAL SPI callbacks above
    // No additional handling needed here
}

/**
 * @brief DMA2 Stream0/3 Error Callback
 * Called when DMA2 Stream0 or Stream3 encounters an error
 */
extern "C" void HAL_DMA_XferErrorCallback(DMA_HandleTypeDef *hdma)
{
    // This is called by the HAL SPI callbacks above  
    // No additional handling needed here
}