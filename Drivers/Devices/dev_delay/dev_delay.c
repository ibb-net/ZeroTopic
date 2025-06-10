#include "dev_basic.h"
#include "gd32h757z_start.h"
#define TIM_MSEC_DELAY        0x01U
#define TIM_USEC_DELAY        0x02U
#define DELAY_TIMER           TIMER50
#define DELAY_TIMER_RCU       RCU_TIMER50
#define DELAY_TIMER_IRQn      TIMER50_IRQn
#define DELAY_TIMER_PRESCALER 23U

__IO uint32_t delay_time      = 0U;
__IO uint16_t timer_prescaler = 23U;
__IO uint32_t timeout_flg     = 0;
static void hw_time_set(uint8_t unit, uint32_t ntime);
static void hw_delay(uint32_t ntime, uint8_t unit);

/*!
    \brief      initializes delay unit using Timer2
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DevDelayInit(void) {
    /* enable the TIM2 global interrupt */
    nvic_irq_enable(DELAY_TIMER_IRQn, 1U, 0U);
    rcu_periph_clock_enable(DELAY_TIMER_RCU);
}
/*!
    \brief      configures DELAY_TIMER for delay routine based on DELAY_TIMER
    \param[in]  unit: msec /usec
    \param[out] none
    \retval     none
*/
static void hw_time_set(uint8_t unit, uint32_t ntime) {
    timer_parameter_struct timer_basestructure;
    timer_deinit(DELAY_TIMER);
    timer_struct_para_init(&timer_basestructure);

    timer_disable(DELAY_TIMER);
    timer_interrupt_disable(DELAY_TIMER, TIMER_INT_UP);
    if (TIM_USEC_DELAY == unit) {
        timer_basestructure.period    = (uint64_t)((10 - 1) * ntime);
        timer_basestructure.prescaler = 30 - 1;
        ;
    } else if (TIM_MSEC_DELAY == unit) {
        timer_basestructure.period    = (uint64_t)(100 - 1);
        timer_basestructure.prescaler = 3000 - 1;
        ;
    } else {
        /* no operation */
    }

    timer_basestructure.alignedmode       = TIMER_COUNTER_EDGE;
    timer_basestructure.counterdirection  = TIMER_COUNTER_UP;
    timer_basestructure.clockdivision     = TIMER_CKDIV_DIV1;
    timer_basestructure.repetitioncounter = 0U;
    timer_init(DELAY_TIMER, &timer_basestructure);
    timer_interrupt_flag_clear(DELAY_TIMER, TIMER_INT_UP);
    timer_auto_reload_shadow_enable(DELAY_TIMER);
    /* TIMER IT enable */
    timer_interrupt_enable(DELAY_TIMER, TIMER_INT_UP);
    /* DELAY_TIMER enable counter */
    timer_enable(DELAY_TIMER);
}
/*!
    \brief      delay routine based on DELAY_TIMER
    \param[in]  ntime: delay Time
    \param[in]  unit: delay Time unit = miliseconds / microseconds
    \param[out] none
    \retval     none
*/
static void hw_delay(uint32_t ntime, uint8_t unit) {
    timeout_flg = 1;
    hw_time_set(unit, ntime);
    while (timeout_flg) {
    }
    timer_disable(DELAY_TIMER);
}

void DevDelayUs(uint16_t ntime) {
    // hw_time_set(TIM_USEC_DELAY);
    // hw_delay(ntime, TIM_USEC_DELAY);
    uint16_t max;
    if (ntime <= 2) {
        return;
    } else if (ntime <= 10) {
        max = 35;
        for (uint16_t i = 0; i < ntime; i++) {
            for (volatile uint16_t j = 0; j < max; j++) {
            }
        }
    } else if (ntime <= 20) {
        max = 38;
        for (uint16_t i = 0; i < ntime; i++) {
            for (volatile uint16_t j = 0; j < max; j++) {
            }
        }
    } else if (ntime <= 50) {
        max = 41;
        for (uint16_t i = 0; i < ntime; i++) {
            for (volatile uint16_t j = 0; j < max; j++) {
            }
        }
    } else if (ntime <= 100) {
        max = 42;
        for (uint16_t i = 0; i < ntime; i++) {
            for (volatile uint16_t j = 0; j < max; j++) {
            }
        }
    } else if (ntime <= 200) {
        max = 42;
        for (uint16_t i = 0; i < ntime; i++) {
            for (volatile uint16_t j = 0; j < max; j++) {
            }
        }
    }else {
        max = 42;
        for (uint16_t i = 0; i < ntime; i++) {
            for (volatile uint16_t j = 0; j < max; j++) {
            }
        }
    }
}

void DevDelayMs(uint32_t ntime) {
    // hw_time_set(TIM_MSEC_DELAY);
    hw_delay(ntime, TIM_MSEC_DELAY);
}

void delay_timer_irq(void) {
    // SCB_CleanDCache_by_Addr((uint32_t *)&delay_time, sizeof(delay_time));

    // if (delay_time > 0x00U) {
    //     delay_time--;
    // } else {
    //     timer_disable(DELAY_TIMER);
    // }
}

void TIMER50_IRQHandler(void) {
    if (SET == timer_interrupt_flag_get(TIMER50, TIMER_INT_FLAG_UP)) {
        /* clear update interrupt bit */
        timer_interrupt_flag_clear(TIMER50, TIMER_INT_FLAG_UP);
        // delay_timer_irq();
        timeout_flg = 0;
    }
}
