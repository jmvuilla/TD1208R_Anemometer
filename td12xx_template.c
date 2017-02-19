#include <efm32.h>
#include <td_core.h>

/* This file declare all "dynamic" library data. It should be last included file
 * Standard size value can be override before including this file
 */
#include <td_config.h>

/* Start Addition by JMV */
#include <stdint.h>
#include <stdbool.h>

#include <efm32.h>
#include <em_gpio.h>
#include <em_pcnt.h>
#include <td_measure.h>
#include <td_core.h>
#include <td_rtc.h>
#include <td_gpio.h>
#include <td_sigfox.h>

uint8_t number_5S=0;
uint8_t number_sample=0;
uint8_t transmission_buffer[12];
uint8_t sendmsg = 0;
#define LED_PORT	TIM2_PORT			/**< LED port */
#define LED_BIT		TIM2_BIT			/**< LED bit */
#define ANEMOMETER_PORT	USR2_PORT				/**< Button port, previously was _PORT */
#define ANEMOMETER_BIT	USR2_BIT				/**< Button bit */
#define ANEMOMETER_MASK	USR2_MASK				/**< Button mask */
uint8_t Every5STimer = 0xFF;
/* End of addition JMV */

static void Every5SFunction(uint32_t arg, uint8_t repetition){
	if (number_5S==0) {
		transmission_buffer[number_sample] = 0;
	} else {
		transmission_buffer[number_sample] += (PCNT_CounterGet(PCNT0)) & 0xff;
	}

	PCNT_CounterTopSet(PCNT0, 0, 255);

	if (number_5S==11) {
		transmission_buffer[number_sample] = (uint8_t)(transmission_buffer[number_sample]*0.067056f);
		if (number_sample==9) {
			int32_t volt = TD_MEASURE_VoltageTemperatureExtended(false);
			transmission_buffer[10] = ( volt >> 4 ) & 0xff;;
			int32_t temp = TD_MEASURE_VoltageTemperatureExtended(true);
			temp = (temp<-300)?-300:temp;
			temp = (temp> 720)? 720:temp;
			temp += 300;
			transmission_buffer[11] = ( temp >> 2 ) & 0xff;
			sendmsg = 1;
			number_sample = 0;
		} else {
			number_sample = number_sample + 1;
		}
		number_5S = 0;
	} else {
		number_5S++;
	}
}

void TD_USER_Setup(void){
	uint8_t i;

	CMU_ClockEnable(cmuClock_PCNT0, true);
	GPIO_PinModeSet(ANEMOMETER_PORT, ANEMOMETER_BIT, gpioModeInputPullFilter, 1);
	GPIO_PinModeSet(LED_PORT, LED_BIT, gpioModePushPull, 1);

	PCNT_Init_TypeDef pcntInit =
		  {
		    .mode       = pcntModeOvsSingle,  /* clocked by LFACLK */
		    .counter    = 0,                  /* Set initial value to 0 */
		    .top        = 255,                 /* Set top to max value */
		    .negEdge    = true,              /* positive edges */
		    .countDown  = false,              /* up count */
		    .filter     = true,               /* filter enabled */
		  };
	PCNT_Init(PCNT0, &pcntInit);
	PCNT0->ROUTE = PCNT_ROUTE_LOCATION_LOC2;

	// Blink the led 5 times at the beginning
	for(i=0;i<6;i++) {
		GPIO_PinOutToggle(LED_PORT, LED_BIT);
		TD_RTC_Delay(T1MS);
		GPIO_PinOutToggle(LED_PORT, LED_BIT);
		TD_RTC_Delay(T1S);
	}

	// Start 60s timer, which will trigger EveryMNFunction every minute
	Every5STimer = TD_SCHEDULER_AppendIrq(5, 0 , 0, TD_SCHEDULER_INFINITE, Every5SFunction, 0);
}

void TD_USER_Loop(void){
	GPIO_PinOutToggle(LED_PORT, LED_BIT);
	if (sendmsg==1) {
		sendmsg = 0;
		TD_SIGFOX_Send(transmission_buffer, 12, 1);
	}
}
