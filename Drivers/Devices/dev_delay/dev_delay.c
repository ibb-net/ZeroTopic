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
static void hw_time_set(uint8_t unit);
static void hw_delay(uint32_t ntime, uint8_t unit);

/*!
    \brief      initializes delay unit using Timer2
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DevDelayInit(void) {
    /* configure the priority group to 2 bits */
    // nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    /* enable the TIM2 global interrupt */
    nvic_irq_enable(DELAY_TIMER_IRQn, 1U, 0U);
    rcu_periph_clock_enable(DELAY_TIMER_RCU);
#if 0
    /* -----------------------------------------------------------------------
   TIMER50 configuration:
   TIMER50 frequency = 300MHz/1500 = 200KHz, the period is 1s(200000/200000 = 1s).
   ----------------------------------------------------------------------- */
   nvic_irq_enable(TIMER50_IRQn, 0,0);
    timer_parameter_struct timer_initpara;
    /* enable the peripherals clock */
    rcu_periph_clock_enable(RCU_TIMER50);
    
    /* deinit a TIMER */
    timer_deinit(TIMER50);
    /* initialize TIMER init parameter struct */
    timer_struct_para_init(&timer_initpara);
    /* TIMER50 configuration */
    timer_initpara.prescaler         = 1500 - 1;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = (uint64_t)(200000 - 1);
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER50, &timer_initpara);

    timer_interrupt_flag_clear(TIMER50, TIMER_INT_FLAG_UP);
    /* enable the TIMER interrupt */
    timer_interrupt_enable(TIMER50, TIMER_INT_UP);
    /* enable a TIMER */
    timer_enable(TIMER50);

#endif
}
/*!
    \brief      configures DELAY_TIMER for delay routine based on DELAY_TIMER
    \param[in]  unit: msec /usec
    \param[out] none
    \retval     none
*/
static void hw_time_set(uint8_t unit) {
#if 1
    /* TIMER50 configuration:
    TIMER50 frequency = 300MHz/1500 = 200KHz, the period is 1s(200000/200000 = 1s). */

    /*
    prescaler = 主时钟频率 / 目标定时器频率 - 1
    prescaler = 600MHz / 1MHz - 1 = 699

    period = 1us * 定时器频率 - 1
    period = 1 * 1MHz - 1 = 0
    */
    timer_parameter_struct timer_basestructure;
    timer_deinit(DELAY_TIMER);
    timer_struct_para_init(&timer_basestructure);

    timer_disable(DELAY_TIMER);
    timer_interrupt_disable(DELAY_TIMER, TIMER_INT_UP);
    if (TIM_USEC_DELAY == unit) {

        timer_basestructure.period = (uint64_t)(10-1);  
        timer_basestructure.prescaler         = 30 -1;
        ;
    } else if (TIM_MSEC_DELAY == unit) {
        timer_basestructure.period = (uint64_t)(100 - 1);
        timer_basestructure.prescaler         = 3000 - 1;
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

#else
    timer_parameter_struct timer_initpara;
    /* enable the peripherals clock */
    rcu_periph_clock_enable(RCU_TIMER50);

    /* deinit a TIMER */
    timer_deinit(TIMER50);
    /* initialize TIMER init parameter struct */
    timer_struct_para_init(&timer_initpara);
    /* TIMER50 configuration */
    timer_initpara.prescaler         = 1500 - 1;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = (uint64_t)(200000 - 1);
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;
    timer_init(TIMER50, &timer_initpara);

    timer_interrupt_flag_clear(TIMER50, TIMER_INT_FLAG_UP);
    /* enable the TIMER interrupt */
    timer_interrupt_enable(TIMER50, TIMER_INT_UP);
    /* enable a TIMER */
    timer_enable(TIMER50);
#endif
}
/*!
    \brief      delay routine based on DELAY_TIMER
    \param[in]  ntime: delay Time
    \param[in]  unit: delay Time unit = miliseconds / microseconds
    \param[out] none
    \retval     none
*/
static void hw_delay(uint32_t ntime, uint8_t unit) {
    delay_time = ntime;
    hw_time_set(unit);
    while (0U != delay_time) {
    }
    timer_disable(DELAY_TIMER);
}

void DevDelayUs(uint32_t ntime) {
    // hw_time_set(TIM_USEC_DELAY);
    hw_delay(ntime, TIM_USEC_DELAY);
}

void DevDelayMs(uint32_t ntime) {
    // hw_time_set(TIM_MSEC_DELAY);
    hw_delay(ntime, TIM_MSEC_DELAY);
}

void delay_timer_irq(void) {
    // if (RESET != timer_interrupt_flag_get(DELAY_TIMER, TIMER_INT_UP)) {
    //     timer_interrupt_flag_clear(DELAY_TIMER, TIMER_INT_UP);

    // }
    if (delay_time > 0x00U) {
        delay_time--;
    } else {
        timer_disable(DELAY_TIMER);
    }
}
void TIMER2_IRQHandler(void) {
    delay_timer_irq();
}

void TIMER50_IRQHandler(void) {
    if (SET == timer_interrupt_flag_get(TIMER50, TIMER_INT_FLAG_UP)) {
        /* clear update interrupt bit */
        timer_interrupt_flag_clear(TIMER50, TIMER_INT_FLAG_UP);
        /* toggle selected led */
        // gd_eval_led_toggle(LED2);
        delay_timer_irq();
    }
}