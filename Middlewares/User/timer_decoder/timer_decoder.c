
#include "timer_decoder_cfg.h"

#if TIMER_DECODER_EN
#include "gd32h7xx.h"
#include <stdio.h>
// #include "gd32h757z_start.h"
#include "timer_decoder.h"

#include "os_server.h"
#include "elog.h"



void timer_encoder_init_config(void)
{
	printf("timer_encoder_init_config\r\n");
	timer_parameter_struct timer_initpara;

	rcu_periph_clock_enable(ENCODER_TIMER_RCU);
	
	rcu_periph_clock_enable(TIMER_A_CH_RCU);
	rcu_periph_clock_enable(TIMER_B_CH_RCU);
	
	gpio_af_set(TIMER_A_CH_PORT,GPIO_AF_13,TIMER_A_CH_PIN);
	gpio_af_set(TIMER_B_CH_PORT,GPIO_AF_13,TIMER_B_CH_PIN);
	
	gpio_mode_set(TIMER_A_CH_PORT,GPIO_MODE_AF,GPIO_PUPD_NONE,TIMER_A_CH_PIN);
	gpio_mode_set(TIMER_B_CH_PORT,GPIO_MODE_AF,GPIO_PUPD_NONE,TIMER_B_CH_PIN);
	
	
	gpio_output_options_set(TIMER_A_CH_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_60MHZ,TIMER_A_CH_PIN);
	gpio_output_options_set(TIMER_B_CH_PORT,GPIO_OTYPE_PP,GPIO_OSPEED_60MHZ,TIMER_B_CH_PIN);
	
	

	timer_deinit(ENCODER_TIMER);

	/* TIMER2 configuration */
	timer_initpara.prescaler         = 0;
	timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
	timer_initpara.counterdirection  = TIMER_COUNTER_UP;
	timer_initpara.period            = 65535;
	timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
	timer_initpara.repetitioncounter = 0;
	timer_init(ENCODER_TIMER,&timer_initpara);
	
	//TIMER_IC_POLARITY_RISING：信号不反相
	//TIMER_IC_POLARITY_FALLING:信号反相
	//timer_quadrature_decoder_mode_config函数的参数ic0polarity对应CI0FE0，ic1polarity对应CI1FE1
	timer_quadrature_decoder_mode_config(ENCODER_TIMER,TIMER_QUAD_DECODER_MODE2,TIMER_IC_POLARITY_RISING,TIMER_IC_POLARITY_RISING);

	/* auto-reload preload enable */
	timer_auto_reload_shadow_enable(ENCODER_TIMER);
	
	//设置初始计数值为0x50，用于判断计数个数和计数方向
	timer_counter_value_config(ENCODER_TIMER,DEFAULT_TIMER_VALUE);

	/* TIMER2 counter enable */
	timer_enable(ENCODER_TIMER);
}
SYSTEM_REGISTER_INIT(BoardInitStage, MCUPre_TIMER_ENCODER_INIT, timer_encoder_init_config, timer_encoder_init_config);
// SYSTEM_REGISTER_INIT(BoardInitStage, MCUPre_TIMER_ENCODER_INIT, timer_encoder_init_config, timer_encoder_init_config);

#endif // TIMER_DECODER_EN