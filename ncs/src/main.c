/** @file main.h
 * @brief Main function definition
 * 
 * @author Gonçalo Peralta & João Alvares
 * @date 03 June 2024
 * @bug No known bugs.
*/

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "../includes/cmdproc.h"
#include "../includes/funcs.h"

// Config ADC
#define SLEEP_TIME_MS 	1000
#define ADC_NODE		DT_NODELABEL(adc)		// DT_N_S_soc_S_adc_40007000
#define ADC_RESOLUTION	10
#define ADC_CHANNEL 	0
#define ADC_PORT 		SAADC_CH_PSELP_PSELP_AnalogInput0	// AIN0
#define ADC_REFERENCE	ADC_REF_INTERNAL					// 0.6v
#define ADC_GAIN 		ADC_GAIN_1_5						// ADC_REFERENCE*5
static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);
struct adc_channel_cfg chl0_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQ_TIME_DEFAULT,
	.channel_id = ADC_CHANNEL,
#ifdef	CONFIG_ADC_NRFX_SAADC
	.input_positive = ADC_PORT
#endif
};
int16_t sample_buffer[4];
struct adc_sequence sequence = {
	/* canais individuais serão adicionados abaixo */
	.channels 	 = BIT(ADC_CHANNEL),
	.buffer		 = sample_buffer,
	
	.buffer_size = sizeof(sample_buffer),
	.resolution  = ADC_RESOLUTION
};

// Config threads
#define THREAD0_PRIORITY 7
#define THREAD1_PRIORITY 7
K_MUTEX_DEFINE(test_mutex);

// Buttons 1-4
const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0});
const struct gpio_dt_spec button_3 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw2), gpios, {0});
const struct gpio_dt_spec button_4 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw3), gpios, {0});

// LEDs 1-4
const struct gpio_dt_spec led_1 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
const struct gpio_dt_spec led_2 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0});		
const struct gpio_dt_spec led_3 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led2), gpios, {0});		
const struct gpio_dt_spec led_4 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led3), gpios, {0});		

// UART
#define STACKSIZE 2048
#define RECEIVE_BUFF_SIZE 20
#define TRANSMIT_BUFF_SIZE 50
#define RECEIVE_TIMEOUT 100
const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));
static uint8_t tx_buf[TRANSMIT_BUFF_SIZE] = "[UART] This is a UART test msg\n";
static uint8_t rx_buf[RECEIVE_BUFF_SIZE] = {0};

// Vars
RTDB database;
int period = 500000; // Frequency of update of RTDB
void updateFreq(int x){
	period = x;
}

// UART Call-back, in case the buffer gets full. rx_buf will act as a circular buffer
static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data){
	switch(evt->type){
		case UART_TX_DONE:
			break;
		case UART_RX_DISABLED:
			uart_rx_enable(dev, rx_buf, sizeof rx_buf, RECEIVE_TIMEOUT);
			break;
		default:
			break;
	}
}

// Thread de atualização da RTDB
void thread0(void){
    initRTDB(&database);
	int err;
	k_busy_wait(500000);

	printk("[TH0] Ready\n");
	while(1){
		err = adc_read(adc_dev, &sequence);
		if(err != 0){
			printk("ADC reading failed with error %d. \n", err);
		}

		k_mutex_lock(&test_mutex, K_FOREVER);	// Refreshing the RTDB Lock the access

		// Buttons
		database.but[0] = gpio_pin_get_dt(&button_1);
		database.but[1] = gpio_pin_get_dt(&button_2);
		database.but[2] = gpio_pin_get_dt(&button_3);
		database.but[3] = gpio_pin_get_dt(&button_4);
		// LEDs
		gpio_pin_set_dt(&led_1, database.led[0]);
		gpio_pin_set_dt(&led_2, database.led[1]);
		gpio_pin_set_dt(&led_3, database.led[2]);
		gpio_pin_set_dt(&led_4, database.led[3]);
		// Analog Read
		database.anRaw = sample_buffer[0];
		database.anVal = (float)database.anRaw * 3/(pow(2, ADC_RESOLUTION)); // Swaping scales where 3 = VDD

		k_mutex_unlock(&test_mutex);			// Refresh done, time to unlock
		printk("here\n");
		k_busy_wait(period);
	}
}

// Thread para enviar e receber códigos
// Commands
	// # B [CS] ! 					- Read button state
	// # L [1/2/3/4] [CS] ! 		- Toggle LED state (Ligado ou desligado)
	// # A [CS] ! 					- Read Analog sensor (Temperatura)
	// # U [00] [CS] !	 			- Change frequecy of update of the in/out digital signals of RTDB
	// So fiz os de cima
	// # S [0000] [CS] ! 			- Change frequecy of sampling of analog input signal
	// # P [CS] ! 					- Toggle Analog reading mode
void thread1(void){
	if(!initHardware()){
        printk("[TH1] Error initilizing Hardware\n");
    }
	int endSym = 0; 	// Holds the position of the EOF_SYM (!)
	int startSym = 0;	// Holds the position of the SOF_SYM (#)
	int flag = 0;		// Goes to 1 if a # and ! are found
	int err = 0;		// Error var handler
	char resp[RECEIVE_BUFF_SIZE];	// Response command
	char cmd[RECEIVE_BUFF_SIZE] = "\0";	// Received command
	char c = '0';		// Dummy char
	memset(rx_buf, 0, sizeof(rx_buf));
	memset(tx_buf, 0, sizeof(tx_buf));

	printk("[TH1] Ready\n");
    while(1){
		endSym = 0; startSym = 0; flag = 0; // Clear all vars 
		
		// Look for a EOF_SYM and store its position
		// if found look for a SOF_SYM and store its position
		// if also found means theres a possible command in the buffer so set flag to 1
		// It also takes into account that rx_buf is circular
		for(int i = 0; i < RECEIVE_BUFF_SIZE; i++){
			if(rx_buf[i] == EOF_SYM){
				endSym = i;
				startSym = endSym;
				for(int j = 0; j < RECEIVE_BUFF_SIZE; j++){
					if(rx_buf[startSym] == SOF_SYM){
						flag = 1;
						break;
					}
					startSym--;
					startSym = startSym == -1 ? RECEIVE_BUFF_SIZE-1 : startSym;
				}
				break;
			}
		}

		// If theres a command start processing
		if(flag == 1){
			// Next for loop creates a substring from the rx_buf with only the Command chars (from # to !)
			for(int i = startSym; i != endSym+1; i++){
				if(i == RECEIVE_BUFF_SIZE){
					i = 0;
				}
				c = rx_buf[i];
				strncat(cmd, &c, 1);
				rx_buf[i] = 'X'; // Mark the command in rx_buf with X's
			}

			// Add a NULL terminator to finalize the string
			if(endSym > startSym){
				cmd[endSym+1] = '\0';
			}
			else{
				cmd[RECEIVE_BUFF_SIZE-startSym+endSym+1] = '\0';
			}

			// Proccess the command
			err = cmdProcessor(cmd, resp, &database);
			if(err == SUCCESS){
				strcpy(tx_buf, resp);
				printk("%s\n", tx_buf);
			} else{
				consoleLog(err);
			}

			memset(cmd, 0, sizeof(cmd));
			memset(tx_buf, 0, sizeof(cmd));
			flag = 0;	// Command was processed so set the flag to zero again
		}
		
		// for(int i = 0; i < RECEIVE_BUFF_SIZE; i++){
		// 	printk("%c", rx_buf[i]);
		// }
		// printk("\n");
		k_busy_wait(5000);
    }
}

K_THREAD_DEFINE(thread0_id, STACKSIZE, thread0, NULL, NULL, NULL, THREAD0_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread1_id, STACKSIZE, thread1, NULL, NULL, NULL, THREAD1_PRIORITY, 0, 0);

int initHardware(){
    int returnValue = 0;

	// Check if the buttons are ready
	if(!gpio_is_ready_dt(&button_1)){
		printk("[NCS] Error: button device %s is not ready\n", button_1.port->name);
		return 0;
	}
	if(!gpio_is_ready_dt(&button_2)){
		printk("[NCS] Error: button device %s is not ready\n", button_2.port->name);
		return 0;
	}
	if(!gpio_is_ready_dt(&button_3)){
		printk("[NCS] Error: button device %s is not ready\n", button_3.port->name);
		return 0;
	}
	if(!gpio_is_ready_dt(&button_4)){
		printk("[NCS] Error: button device %s is not ready\n", button_4.port->name);
		return 0;
	}
	
	// Configure the buttons as inputs
	returnValue = gpio_pin_configure_dt(&button_1, GPIO_INPUT);
	if(returnValue != 0){
		printk("[NCS] Error %d: failed to configure %s pin %d\n", returnValue, button_1.port->name, button_1.pin);
		return 0;
	}
	returnValue = gpio_pin_configure_dt(&button_2, GPIO_INPUT);
	if(returnValue != 0){
		printk("[NCS] Error %d: failed to configure %s pin %d\n", returnValue, button_2.port->name, button_2.pin);
		return 0;
	}
	returnValue = gpio_pin_configure_dt(&button_3, GPIO_INPUT);
	if(returnValue != 0){
		printk("[NCS] Error %d: failed to configure %s pin %d\n", returnValue, button_3.port->name, button_3.pin);
		return 0;
	}
	returnValue = gpio_pin_configure_dt(&button_4, GPIO_INPUT);
	if(returnValue != 0){
		printk("[NCS] Error %d: failed to configure %s pin %d\n", returnValue, button_4.port->name, button_4.pin);
		return 0;
	}
	printk("[NCS] Set up button_1 at %s pin %d\n", button_1.port->name, button_1.pin);
	printk("[NCS] Set up button_2 at %s pin %d\n", button_2.port->name, button_2.pin);
	printk("[NCS] Set up button_3 at %s pin %d\n", button_3.port->name, button_3.pin);
	printk("[NCS] Set up button_4 at %s pin %d\n", button_4.port->name, button_4.pin);

	// Check if the LEDs are ready
	if(led_1.port && !gpio_is_ready_dt(&led_1)){
		printk("[NCS] Error %d: LED_1 device %s is not ready\n", returnValue, led_1.port->name);
		return 0;
	}
	if(led_2.port && !gpio_is_ready_dt(&led_2)){
		printk("[NCS] Error %d: LED_2 device %s is not ready\n", returnValue, led_2.port->name);
		return 0;
	}
	if(led_3.port && !gpio_is_ready_dt(&led_3)){
		printk("[NCS] Error %d: LED_3 device %s is not ready\n", returnValue, led_3.port->name);
		return 0;
	}
	if(led_4.port && !gpio_is_ready_dt(&led_4)){
		printk("[NCS] Error %d: LED_4 device %s is not ready\n", returnValue, led_4.port->name);
		return 0;
	}

	// Configuring LEDs as outputs
	returnValue = gpio_pin_configure_dt(&led_1, GPIO_OUTPUT);
	if(returnValue != 0){
		printk("[NCS] Error %d: failed to configure LED_1 device %s pin %d\n", returnValue, led_1.port->name, led_1.pin);
		return 0;
	} else{
		printk("[NCS] Set up LED_1 at %s pin %d\n", led_1.port->name, led_1.pin);
	}
	returnValue = gpio_pin_configure_dt(&led_2, GPIO_OUTPUT);
	if(returnValue != 0){
		printk("[NCS] Error %d: failed to configure LED_1 device %s pin %d\n", returnValue, led_2.port->name, led_2.pin);
		return 0;
	} else{
		printk("[NCS] Set up LED_2 at %s pin %d\n", led_2.port->name, led_2.pin);
	}
	returnValue = gpio_pin_configure_dt(&led_3, GPIO_OUTPUT);
	if(returnValue != 0){
		printk("[NCS] Error %d: failed to configure LED_1 device %s pin %d\n", returnValue, led_3.port->name, led_3.pin);
		return 0;
	} else{
		printk("[NCS] Set up LED_3 at %s pin %d\n", led_3.port->name, led_3.pin);
	}
	returnValue = gpio_pin_configure_dt(&led_4, GPIO_OUTPUT);
	if(returnValue != 0){
		printk("[NCS] Error %d: failed to configure LED_1 device %s pin %d\n", returnValue, led_4.port->name, led_4.pin);
		return 0;
	} else{
		printk("[NCS] Set up LED_4 at %s pin %d\n", led_4.port->name, led_4.pin);
	}

	// Turning LEDs off
	gpio_pin_set_dt(&led_1, 0);
	gpio_pin_set_dt(&led_2, 0);
	gpio_pin_set_dt(&led_3, 0);
	gpio_pin_set_dt(&led_4, 0);

    // Check if UART is ready
    if(!device_is_ready(uart)){
        printk("[NCS] Error: UART device not ready\n");
    }
    printk("[NCS] Device UART ready\n");
	returnValue = uart_callback_set(uart, uart_cb, NULL);
	if(returnValue){
		printk("[NCS] Error: Failled to set up UART callback\n");
		return 0;
	}
	returnValue = uart_tx(uart, tx_buf, sizeof(tx_buf), SYS_FOREVER_MS);
	if(returnValue){
		printk("[NCS] Error: Failled to send TX buffer\n");
		return 0;
	}
	returnValue = uart_rx_enable(uart, rx_buf, sizeof(rx_buf), RECEIVE_TIMEOUT);
	if(returnValue){
		printk("[NCS] Error: Failled to set up UART\n");
		return 0;
	}
	printk("[NCS] UART device ready\n");

	// Check if ADC is ready
	if(!device_is_ready(adc_dev)){
		printk("[NCS] Error: ADC device not ready\n");
	}
	returnValue = adc_channel_setup(adc_dev, &chl0_cfg);
	if(returnValue != 0){
		printk("[NCS] Error: ADC adc_channel_setup failed with error %d.\n", returnValue);
	}
	printk("[NCS] ADC device ready\n");
	return 1;
}

// void initRTDB(RTDB *rtdb){
//     rtdb->led[0] = 0;
//     rtdb->led[1] = 0;
//     rtdb->led[2] = 0;
//     rtdb->led[3] = 0;
//     rtdb->but[0] = 0;
//     rtdb->but[1] = 0;
//     rtdb->but[2] = 0;
//     rtdb->but[3] = 0;
//     rtdb->anRaw = 0;
//     rtdb->anVal = 0;
// }