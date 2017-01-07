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
#include <td_measure.h>
#include <td_core.h>
#include <td_rtc.h>
#include <td_gpio.h>
#include <td_sigfox.h>

uint8_t number_sample=0;
uint8_t transmission_buffer[12];
uint8_t sendmsg = 0;
#define LED_PORT	TIM2_PORT			/**< LED port */
#define LED_BIT		TIM2_BIT			/**< LED bit */
#define ANEMOMETER_PORT	USR2_PORT				/**< Button port, previously was _PORT */
#define ANEMOMETER_BIT	USR2_BIT				/**< Button bit */
#define ANEMOMETER_MASK	USR2_MASK				/**< Button mask */
uint8_t EveryMNTimer = 0xFF;
uint32_t anemometer_counter=0;
/* End of addition JMV */

static void EveryMNFunction(uint32_t arg, uint8_t repetition){
	// change line below - anemometer_counter * 0.067056 (to convert count into km/h)
	transmission_buffer[number_sample] = (anemometer_counter) & 0xff;
	anemometer_counter = 0;

	if (number_sample==9) {
		int32_t volt = TD_MEASURE_VoltageTemperatureExtended(false);
		transmission_buffer[10] = ( volt >> 8 ) & 0xff;;
		transmission_buffer[11] = ( volt ) & 0xff;
		sendmsg = 1;
		number_sample = 0;
	} else {
		number_sample = number_sample + 1;
	}
}

static void AnemometerInterrupt(uint32_t mask){
	anemometer_counter++;
}

void TD_USER_Setup(void){
	int type;
	int i;
	IRQn_Type irq_parity;

	// Setup TIM2_PORT as the LED I/O
	GPIO_PinModeSet(LED_PORT, LED_BIT, gpioModePushPull, 1);

	// Blink the led 5 times at the beginning
	for(i=0;i<6;i++) {
		GPIO_PinOutToggle(LED_PORT, LED_BIT);
		TD_RTC_Delay(T1MS);
		GPIO_PinOutToggle(LED_PORT, LED_BIT);
		TD_RTC_Delay(T1S);
	}

	// Setup ANEMOMETER_PORT as the ANEMOMETER I/O
	GPIO_PinModeSet(ANEMOMETER_PORT, ANEMOMETER_BIT, gpioModeInputPullFilter, 1);

	// Set up a user hook on button pin interrupt
	type = (ANEMOMETER_MASK & TD_GPIO_ODD_MASK) ?
			TD_GPIO_USER_ODD : TD_GPIO_USER_EVEN;
	TD_GPIO_SetCallback(type, AnemometerInterrupt, ANEMOMETER_MASK);
	// Enable falling edge interrupts on button pin
	GPIO_IntConfig(ANEMOMETER_PORT, ANEMOMETER_BIT, false, true, true);
	// Clear and enable the corresponding interrupt in the CPU's Nested Vector
	// Interrupt Controller
	irq_parity =
			(ANEMOMETER_MASK & TD_GPIO_ODD_MASK) ? GPIO_ODD_IRQn : GPIO_EVEN_IRQn;
	NVIC_ClearPendingIRQ(irq_parity);
	NVIC_EnableIRQ(irq_parity);

	// Start 60s timer, which will trigger EveryMNFunction every minute
	EveryMNTimer = TD_SCHEDULER_AppendIrq(60, 0 , 0, TD_SCHEDULER_INFINITE, EveryMNFunction, 0);
}

void TD_USER_Loop(void){
	GPIO_PinOutToggle(LED_PORT, LED_BIT);
	if (sendmsg==1) {
		sendmsg = 0;
		TD_SIGFOX_Send(transmission_buffer, 12, 1);
	}
}
