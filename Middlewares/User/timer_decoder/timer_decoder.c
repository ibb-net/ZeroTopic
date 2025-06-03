
#include "timer_decoder_cfg.h"

#if CONFIG_TIMER_DECODER_EN
#include "gd32h7xx.h"
#include <stdio.h>
#include "timer_decoder.h"

#include "os_server.h"
#include "elog.h"
#define TAG "ENCODER"

typedef struct
{
	uint32_t timer_rcu;
	uint32_t timer_base;
	uint32_t gpio_rcu;
	uint32_t gpio_port;
	uint32_t gpio_af;
	uint32_t gpio_pin_a;
	uint32_t gpio_pin_b;
} TypdefDecoderBSPCfg;
TypdefDecoderBSPCfg decoder_bsp_cfg[CONFIG_TIMER_DECODER_CHANNEL_MAX] = {

	{
		.timer_rcu = ENCODER_CH0_TIMER_RCU,
		.timer_base = ENCODER_CH0_TIMER,
		.gpio_rcu = ENCODER_CH0_GPIO_RCU,
		.gpio_port = ENCODER_CH0_GPIO_PORT,
		.gpio_af = ENCODER_CH0_GPIO_AF,
		.gpio_pin_a = ENCODER_CH0_GPIO_PIN_A,
		.gpio_pin_b = ENCODER_CH0_GPIO_PIN_B,
	},
#if (CONFIG_TIMER_DECODER_CHANNEL_MAX > 1)
	{
		.timer_rcu = ENCODER_CH1_TIMER_RCU,
		.timer_base = ENCODER_CH1_TIMER,
		.gpio_rcu = ENCODER_CH1_GPIO_RCU,
		.gpio_port = ENCODER_CH1_GPIO_PORT,
		.gpio_af = ENCODER_CH1_GPIO_AF,
		.gpio_pin_a = ENCODER_CH1_GPIO_PIN_A,
		.gpio_pin_b = ENCODER_CH1_GPIO_PIN_B,
	},
#endif
};
typedef struct
{
	uint8_t channel; // 通道号 0:CH0 1:CH1
	uint16_t event;
	uint32_t timer_handle;
	uint8_t channel_ctr; // 0 stop 1:start
	uint8_t mode;		 // 0:连续模式 1:步进模式
	uint32_t phy_value;
	uint32_t phy_value_max; // 最大物理值
	uint32_t phy_value_min; // 最小物理值
	uint64_t diff;			// 上次计数值和当前计数值的差值

	uint8_t is_turning;			   // 是否正在旋转
	uint64_t last_encoder_counter; // 上次计数值
	uint64_t start_cnt;			   // 旋转开始计数值
	uint8_t direction;			   // 旋转方向 0:正向 1:反向
	uint32_t tmp_cnt;			   // 当一次读取不足 4个计数值时，临时存储的计数值
	uint32_t active_duration;	   // 通道激活持续时间
	uint32_t active_timeout;	   // 通道激活超时时间
	uint32_t pluse_gain;		   // 旋转速度产生的增益
	uint32_t duration_gain;		   // 旋转时间产生的增益
	uint32_t pluse_cnt;			   // 旋转单次触发的个数
} TypdefEncoderStruct;
TypdefEncoderStruct encoder_struct[CONFIG_TIMER_DECODER_CHANNEL_MAX] = {0};
static void __decoder_handle(void);
static void __encoder_timer_clear(TypdefEncoderStruct *encoder);

void device_timer_encoder_init(void)
{

	printf("device_timer_encoder_init\r\n");
	timer_parameter_struct timer_initpara;

	for (size_t i = 0; i < CONFIG_TIMER_DECODER_CHANNEL_MAX; i++)
	{

		rcu_periph_clock_enable(decoder_bsp_cfg[i].timer_rcu);
		rcu_periph_clock_enable(decoder_bsp_cfg[i].gpio_rcu);
		gpio_af_set(decoder_bsp_cfg[i].gpio_port, decoder_bsp_cfg[i].gpio_af, decoder_bsp_cfg[i].gpio_pin_a);
		gpio_af_set(decoder_bsp_cfg[i].gpio_port, decoder_bsp_cfg[i].gpio_af, decoder_bsp_cfg[i].gpio_pin_b);
		gpio_mode_set(decoder_bsp_cfg[i].gpio_port, GPIO_MODE_AF, GPIO_PUPD_NONE, decoder_bsp_cfg[i].gpio_pin_a);
		gpio_mode_set(decoder_bsp_cfg[i].gpio_port, GPIO_MODE_AF, GPIO_PUPD_NONE, decoder_bsp_cfg[i].gpio_pin_b);
		gpio_output_options_set(decoder_bsp_cfg[i].gpio_port, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, decoder_bsp_cfg[i].gpio_pin_a);
		gpio_output_options_set(decoder_bsp_cfg[i].gpio_port, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, decoder_bsp_cfg[i].gpio_pin_b);

		timer_deinit(decoder_bsp_cfg[i].timer_base); // 复位计时器
		// 初始化计时器结构体
		timer_initpara.prescaler = 0;
		timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
		timer_initpara.counterdirection = TIMER_COUNTER_UP;
		timer_initpara.period = 65535;
		timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
		timer_initpara.repetitioncounter = 0;
		timer_init(decoder_bsp_cfg[i].timer_base, &timer_initpara);																						   // 初始化计时器
		timer_quadrature_decoder_mode_config(decoder_bsp_cfg[i].timer_base, TIMER_QUAD_DECODER_MODE2, TIMER_IC_POLARITY_RISING, TIMER_IC_POLARITY_RISING); // 配置计时器为四分频解码模式
		timer_auto_reload_shadow_enable(ENCODER_CH0_TIMER);
		timer_counter_value_config(ENCODER_CH0_TIMER, DEFAULT_TIMER_VALUE);
		timer_enable(ENCODER_CH0_TIMER); // 使能计时器

		encoder_struct[i].timer_handle = decoder_bsp_cfg[i].timer_base; // 设置计时器句柄
		encoder_struct[i].channel = i;									// 设置通道号
		encoder_struct[i].channel_ctr = 0;								// 初始化通道状态为停止
	}
}
SYSTEM_REGISTER_INIT(BoardInitStage, MCUPre_TIMER_ENCODER_INIT, device_timer_encoder_init, device_timer_encoder_init);

#define ENCODER_TIMER_TASK_PRIO (tskIDLE_PRIORITY + 1)

// 函数声明
static void ENCODER_TIMER_RCV_HANDLE(void *msg);
static void ENCODER_TIMER_CYC_HANDLE(void);
static void ENCODER_TIMER_INIT_HANDLE(void *msg);

// 定义事件列表
static const vfb_event_t ENCODER_TIMER_event_list[] = {
	ENCODER_TIMER_START,
	ENCODER_TIMER_STOP,

};

// 定义任务配置结构体
static const VFBTaskStruct ENCODER_TIMER_task_cfg = {
	.name = "VFBTaskENCODER_TIMER", // Task name
	.pvParameters = NULL,
	.uxPriority = 10,													 // Task parameters
	.queue_num = 8,														 // Number of queues to subscribe
	.event_list = ENCODER_TIMER_event_list,								 // Event list to subscribe
	.event_num = sizeof(ENCODER_TIMER_event_list) / sizeof(vfb_event_t), // Number of events to subscribe
	.startup_wait_event_list = NULL,									 // Events to wait for at startup
	.startup_wait_event_num = 0,										 // Number of startup events to wait for
	.xTicksToWait = pdMS_TO_TICKS(CONFIG_ENCODER_CYCLE_TIMER_MS),		 // Wait indefinitely
	.init_msg_cb = ENCODER_TIMER_INIT_HANDLE,							 // Callback for initialization messages
	.rcv_msg_cb = ENCODER_TIMER_RCV_HANDLE,								 // Callback for received messages
	.rcv_timeout_cb = ENCODER_TIMER_CYC_HANDLE,							 // Callback for timeout
};

// 任务初始化函数
void ENCODER_TIMER_CREATE_HANDLE(void)
{
	for (size_t i = 0; i < CONFIG_TIMER_DECODER_CHANNEL_MAX; i++)
	{
		encoder_struct[i].phy_value = DEF_PHY_VALUE;	  // 初始化物理值
		encoder_struct[i].phy_value_max = MAX_PHY_VALUE;  // 设置最大物理值
		encoder_struct[i].phy_value_min = MIN_PHY_VALUE;  // 设置最小物理值
		encoder_struct[i].mode = DECODER_MODE_CONTINUOUS; // 设置默认模式为连续模式
		__encoder_timer_clear(&encoder_struct[i]);		  // 初始化每个通道的计时器
	}

	xTaskCreate(VFBTaskFrame, "VFBTaskENCODE", configMINIMAL_STACK_SIZE, (void *)&ENCODER_TIMER_task_cfg, ENCODER_TIMER_TASK_PRIO, NULL);
}
SYSTEM_REGISTER_INIT(AppInitStage, 01, ENCODER_TIMER_CREATE_HANDLE, ENCODER_TIMER_CREATE_HANDLE init);

static void __encoder_timer_clear(TypdefEncoderStruct *encoder)
{

	// encoder->phy_value = DEFAULT_TIMER_VALUE;								   // 重置物理值
	encoder->diff = 0;														   // 重置差值
	encoder->is_turning = 0;												   // 重置旋转状态
	encoder->last_encoder_counter = timer_counter_read(encoder->timer_handle); // DEFAULT_TIMER_VALUE; // 重置上次计数值
	encoder->start_cnt = 0;													   // 重置旋转开始计数值
	encoder->direction = 0;													   // 重置旋转方向
	encoder->tmp_cnt = 0;													   // 重置临时计数值
	encoder->active_duration = 0;											   // 重置激活持续时间
	encoder->active_timeout = 0;											   // 重置激活超时时间
	encoder->pluse_gain = CONFIG_ENCODER_PULSE_GAIN_DEFAULT;				   // 重置旋转速度增益
	encoder->duration_gain = CONFIG_ENCODER_DURATION_GAIN_DEFAULT;			   // 重置旋转时间增益
	encoder->pluse_cnt = CONFIG_ENCODER_ONE_PULSE_CNT;						   // 重置旋转单次触发的个数
																			   // elog_i(TAG, "Encoder channel %d Clear", encoder->channel);
}

static void ENCODER_TIMER_INIT_HANDLE(void *msg)
{
	elog_i(TAG, "ENCODER_TIMER_INIT_HANDLE\r\n");
	// 发布事件，表示计时器已准备就绪
	vfb_publish(ENCODER_TIMER_READY);
	vfb_send(ENCODER_TIMER_START, DECODER_CHANNEL0, 0, NULL); // 启动计时器
}
// 接收消息的回调函数
static void ENCODER_TIMER_RCV_HANDLE(void *msg)
{
	TaskHandle_t curTaskHandle = xTaskGetCurrentTaskHandle();
	char *taskName = pcTaskGetName(curTaskHandle);
	vfb_message_t tmp_msg = (vfb_message_t)msg;
	switch (tmp_msg->frame->head.event)
	{
	case ENCODER_TIMER_START:
	{
		DecoderChannelEnum channel = (DecoderChannelEnum)tmp_msg->frame->head.data;
		if (channel >= CONFIG_TIMER_DECODER_CHANNEL_MAX)
		{
			elog_e(TAG, "ENCODER_TIMER_START: Invalid channel %d", channel);
			return; // 无效通道
		}
		elog_i(TAG, "ENCODER_TIMER_START %d", channel);
		encoder_struct[channel].channel_ctr = 1;																 // 启用通道
		encoder_struct[channel].last_encoder_counter = timer_counter_read(encoder_struct[channel].timer_handle); // 读取当前计时器值
	}
	break;
	case ENCODER_TIMER_STOP:
	{
		DecoderChannelEnum channel = (DecoderChannelEnum)tmp_msg->frame->head.data;
		if (channel >= CONFIG_TIMER_DECODER_CHANNEL_MAX)
		{
			elog_e(TAG, "ENCODER_TIMER_STOP: Invalid channel %d", channel);
			return; // 无效通道
		}
		if (encoder_struct[channel].channel_ctr == 0)
		{
			elog_w(TAG, "ENCODER_TIMER_STOP: Channel %d is already stopped", channel);
			return; // 通道已停止
		}
		elog_i(TAG, "ENCODER_TIMER_STOP %d", channel);
		encoder_struct[channel].channel_ctr = 0;																 // 停止通道
		encoder_struct[channel].last_encoder_counter = timer_counter_read(encoder_struct[channel].timer_handle); // 读取当前计时器值
	}
	break;

	default:
		printf("TASK %s RCV: unknown event: %d\r\n", taskName, tmp_msg->frame->head.event);
		break;
	}
}

// 超时处理的回调函数
static void ENCODER_TIMER_CYC_HANDLE(void)
{
	__decoder_handle();
}

static void __decoder_handle(void)
{
	for (size_t i = 0; i < CONFIG_TIMER_DECODER_CHANNEL_MAX; i++)
	{
		if (!encoder_struct[i].channel_ctr)
		{
			continue; // 如果通道未启用，跳过
		}
		int64_t diff_abs = 0;
		int64_t diff = 0;
		uint8_t direction = 0; // 旋转方向
		TickType_t currentTick = xTaskGetTickCount();
		TypdefEncoderStruct *encoder = &encoder_struct[i];
		// 读取当前计时器值
		uint64_t current_encoder_counter = timer_counter_read(encoder_struct[i].timer_handle);
		diff = current_encoder_counter - encoder_struct[i].last_encoder_counter;
		diff_abs = (diff < 0) ? -diff : diff; // 计算绝对差值
		direction = (diff > 0) ? 0 : 1;		  // 计算旋转方向，正向为0，反向为1
		uint32_t tmp_pluse_cnt = 0;			  // 当前周期有效变化

		if (diff_abs)
		{
			// elog_i(TAG, "Encoder channel %d diff: %ld", i, diff);
			// elog_i(TAG, "current_encoder_counter: %lu", current_encoder_counter);
			// elog_i(TAG, " last_encoder_counter: %lu", encoder_struct[i].last_encoder_counter);
			encoder_struct[i].last_encoder_counter = current_encoder_counter; // 更新上次计数值
			// elog_i(TAG, "CH0 diff: %ld", diff);
			// elog_i(TAG, "CH0 Direction: %s", (diff > 0) ? "Forward" : "Backward");
			/* 更新基础状态 */
			encoder->is_turning = 1; // 正在旋转
			encoder->active_duration += CONFIG_ENCODER_CYCLE_TIMER_MS;
			encoder->active_timeout = currentTick + CONFIG_ENCODER_ACTIVE_TIMEOUT_MS;

			/* 判断是否有完成一次旋转,和旋转的个数 */
			encoder->tmp_cnt += diff_abs; // 累加当前周期的计数值
			tmp_pluse_cnt = encoder->tmp_cnt / CONFIG_ENCODER_ONE_PULSE_CNT;
			encoder->tmp_cnt = encoder->tmp_cnt % CONFIG_ENCODER_ONE_PULSE_CNT; // 更新剩余计数值
#if 1
			if (encoder->direction != direction)
			{
				encoder->direction = direction;
				encoder->pluse_gain = 1;
				encoder->duration_gain = 1;
				encoder->active_duration = 0; // 重置激活持续时间
			}
			else
			{
				/* 计算转速增益 */
				if (tmp_pluse_cnt < 5)
				{
					encoder->pluse_gain = 1;
					encoder->active_duration = 0; // 如果转速过低，重置激活持续时间
				}
				else if (tmp_pluse_cnt < 10)
				{
					encoder->pluse_gain = 2;
				}
				else
				{
					encoder->pluse_gain = 10;
				}
#if 1
				/* 计算时间增益 */
				if (encoder->active_duration < CONFIG_ENCODER_CYCLE_TIMER_MS * 30)
				{
					encoder->duration_gain = 1;
				}
				else
				{
					encoder->duration_gain = 10;
				}
#else
				encoder->duration_gain = 1;
#endif
			}

			uint64_t tmp_gain = encoder->pluse_gain * encoder->duration_gain;
			if (diff > 0)
			{
				encoder->phy_value += tmp_pluse_cnt * tmp_gain; // 更新物理值，正向旋转
			}
			else
			{
				if (encoder->phy_value < tmp_pluse_cnt * tmp_gain)
				{
					encoder->phy_value = MIN_PHY_VALUE; // 如果物理值小于增益值，重置为0
				}
				else
					encoder->phy_value -= tmp_pluse_cnt * tmp_gain; // 更新物理值，反向旋转
			}
			if (encoder->phy_value > MAX_PHY_VALUE)
			{
				encoder->phy_value = MAX_PHY_VALUE; // 限制物理值最大值
			}
			else if (encoder->phy_value < MIN_PHY_VALUE)
			{
				encoder->phy_value = MIN_PHY_VALUE; // 限制物理值最小值
			}
			VFBMsgDecoderStruct msg_decoder = {
				.channel = i,					 // 通道号
				.phy_value = encoder->phy_value, // 物理值
			};
			vfb_send(ENCODER_TIMER_GET_PHY, 0, sizeof(VFBMsgDecoderStruct), (void *)&msg_decoder); // 发送物理值消息
																								   // printf("\r %ld\r\n", encoder->phy_value);								// 输出物理值
						 																		   // printf("\r Physical Value: %ld,Pluses %d Gain: %ld, Duration: %lu ms, Pulse Count: %d\r\n",
																								   // 	   encoder->phy_value, tmp_pluse_cnt, tmp_gain, encoder->active_duration, encoder->pluse_cnt);
#endif																							   // 0
		}
		else
		{
			if (encoder->is_turning)
			{
				if (encoder->active_timeout < currentTick)
				{
					__encoder_timer_clear(&encoder_struct[i]); // 如果没有旋转，清除计时器
															   // elog_i(TAG, "Encoder stopped after %d ms of inactivity", encoder->active_duration);
				}
				else
				{
					// do nothing
				}
			}
		}
	}
}
#endif // CONFIG_TIMER_DECODER_EN