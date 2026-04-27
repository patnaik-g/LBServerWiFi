#pragma once

#include "soc/uart_struct.h"
#include "soc/uart_reg.h"
#include "AsyncDebug.h" // For LOG_DEBUG

// --- UART Tuning Constants ---
// Glitch filter width in APB clock cycles. Tuned empirically.
const uint8_t UART_GLITCH_FILTER_CYCLES = 0x20;
// RX FIFO timeout in baud cycles.
const uint8_t UART_RX_TIMEOUT_CYCLES    = 20;

// --- UART Error Watchdog ---
// Pre-calculated mask for the hardware error watchdog.
// UART_BRK_DET_INT_RAW_M is useful for detecting a long bus-off condition.
const uint32_t UART_ERROR_WATCHDOG_MASK = (
    UART_RXFIFO_OVF_INT_RAW_M | 
    UART_FRM_ERR_INT_RAW_M | 
    UART_PARITY_ERR_INT_RAW_M | 
    UART_GLITCH_DET_INT_RAW_M | 
    UART_BRK_DET_INT_RAW_M
);

/**
 * @brief Applies custom tuning to the LocoNet UART (UART1).
 * 
 * This function configures the hardware glitch filter and RX FIFO timeout.
 * It must be called *after* the UART is initialized (e.g., lnStream.start())
 * to ensure settings are not overwritten.
 */
inline void applyUartTuning() {
    uint32_t conf0 = READ_PERI_REG(UART_CONF0_REG(1));

    // 1. Glitch Filter: 0x20 (32 cycles) was found to be the maximum stable value.
    conf0 &= ~(0xFF << UART_GLITCH_FILT_S);
    conf0 |= (UART_GLITCH_FILTER_CYCLES << UART_GLITCH_FILT_S);

    // 2. RX FIFO Timeout: 20 baud cycles (approx 1.2ms) to tolerate inter-byte gaps.
    conf0 &= ~(0xFF << UART_RX_TOUT_THRHD_S);
    conf0 |= (UART_RX_TIMEOUT_CYCLES << UART_RX_TOUT_THRHD_S);
    conf0 |= UART_RX_TOUT_EN; // Enable the RX timeout feature.

    WRITE_PERI_REG(UART_CONF0_REG(1), conf0);
    LOG_DEBUG("UART1 tuned: GlitchFilter=0x%02X, RxTimeout=%d\n", UART_GLITCH_FILTER_CYCLES, UART_RX_TIMEOUT_CYCLES);
}

/**
 * @brief Checks for and clears hardware UART errors.
 * 
 * Reads the UART's raw interrupt status register and logs any detected
 * framing, parity, overflow, glitch, or break errors.
 */
inline void checkUartErrors() {
    // Use a volatile read to ensure we get the latest register value
    volatile uint32_t uart_errs = UART1.int_raw.val;

    if (uart_errs & UART_ERROR_WATCHDOG_MASK) {
        LOG_DEBUG("UART1 ERROR: Triggered by[%s%s%s%s%s]\n",
              (uart_errs & UART_PARITY_ERR_INT_RAW_M) ? " PARITY" : "",
              (uart_errs & UART_FRM_ERR_INT_RAW_M) ? " FRAME" : "",
              (uart_errs & UART_RXFIFO_OVF_INT_RAW_M) ? " OVF" : "",
              (uart_errs & UART_GLITCH_DET_INT_RAW_M) ? " GLITCH" : "",
              (uart_errs & UART_BRK_DET_INT_RAW_M) ? " BREAK" : "");
        
        // Clear the logged interrupts
        UART1.int_clr.val = UART_ERROR_WATCHDOG_MASK;
    }
}