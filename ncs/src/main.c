#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <string.h>

#include "../includes/cmd.h"
#include "../includes/main.h"

//Config ADC
#define SLEEP_TIME_MS 	1000
#define ADC_NODE		DT_NODELABEL(adc)		//DT_N_S_soc_S_adc_40007000
static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);

#define ADC_RESOLUTION	10
#define ADC_CHANNEL 	0
#define ADC_PORT 		SAADC_CH_PSELP_PSELP_AnalogInput0	//AIN0
#define ADC_REFERENCE	ADC_REF_INTERNAL					//0.6v
#define ADC_GAIN 		ADC_GAIN_1_5						//ADC_REFERENCE*5

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


// Config UART
#define STACKSIZE 1024
#define RECEIVE_BUFF_SIZE 20
#define RECEIVE_TIMEOUT 100

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

static uint8_t tx_buf[] =   {"nRF Connect SDK Fundamentals Course\n\r"
                             "Press 1-3 on your keyboard to toggle LEDS 1-3 on your development kit\n\r"};
static uint8_t rx_buf[RECEIVE_BUFF_SIZE] = {0};
static uint8_t rx[RECEIVE_BUFF_SIZE] = {0};        				// Buffer para armazenar dados recebidos
static int n = 0;

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data){
	switch(evt->type){
		case UART_RX_RDY:

			break;
		case UART_RX_DISABLED:
			uart_rx_enable(dev, rx_buf, sizeof(rx_buf), RECEIVE_TIMEOUT);
			break;
		default:
			break;
	}
}


// Struct
static RTDB database;
int initHardware();

int timer = 0;


// Thread de atualização da RTDB
void thread0(void){
    initRTDB(&database);
	bool val[4];
	int16_t raw_value;
	float val_value;
	int erro;
	int time;

	while (1) {
		erro = adc_read(adc_dev, &sequence);
		if (erro != 0) {
			printk("ADC reading failed with error %d. \n", erro);
			return;
		}

		k_mutex_lock(&test_mutex, K_FOREVER);

		val[0] = gpio_pin_get_dt(&button_1);
		database.but[0] = val[0];
		val[1] = gpio_pin_get_dt(&button_2);
		database.but[1] = val[1];
		val[2] = gpio_pin_get_dt(&button_3);
		database.but[2] = val[2];
		val[3] = gpio_pin_get_dt(&button_4);
		database.but[3] = val[3];
		gpio_pin_set_dt(&led_1, database.led[0]);
		gpio_pin_set_dt(&led_2, database.led[1]);
		gpio_pin_set_dt(&led_3, database.led[2]);
		gpio_pin_set_dt(&led_4, database.led[3]);
		raw_value = sample_buffer[0];
		val_value = (float) raw_value * 0.0029;
		val_value = val_value*60-60;
		int32_t val_an = (int32_t) val_value;
		database.anRaw = raw_value;
		database.anVal = val_an;

		k_mutex_unlock(&test_mutex);
		time = timer*1000000;
		//printk ("Tempo de espera: %d\n", time);
		k_busy_wait(time);
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
	
	memset(rx_buf, 0, sizeof(rx_buf));
	int err = 0;
	int i = 0, j = 0;
	uint8_t input_pos = 0;


    while(1) { 
		input_pos = j;

		// Processamento dos carateres recebidos
		// Spent over 30 minutes looking at this and I still have no idea how it works
		while(rx_buf[input_pos] != '\0'){						// while que lê os digitos recebidos
			
			printk("%c", rx_buf[input_pos]);
			j = input_pos+1;
			
			rx[n] = rx_buf[input_pos];
			
			if (rx_buf[input_pos] == EOF_SYM) {					// Quando existir '!' envia código
				if (input_pos == (RECEIVE_BUFF_SIZE-1)){
					input_pos = 0;
					j = 0;
				}
				break;
			}

			input_pos++;
			if (input_pos == RECEIVE_BUFF_SIZE){
				input_pos = 0;
				j = 0;
			}
			n++;
			if(n == RECEIVE_BUFF_SIZE){
				n = 0;
			}			
		}
		
		// Processamento do código enviado (Terminação em '!')
		// yeah good luck with that
		if (rx[n] == EOF_SYM) {				// Se o ultimo caractere nao for '!' nao avança
			i = 0;
			if (rx[i] != SOF_SYM) {			// Se o primeiro caractere nao for '#' nao precisa avançar mais
				for(i=0; i<n+1; i++) {
					printk("%c ", rx[i]);
				}
				memset(rx, 0, sizeof(rx));
				n=0;
				goto end_thread;
			}

			k_mutex_lock(&test_mutex, K_FOREVER);	// lock mutex porque vai usar uma variavel (database) que thread0 tambem utiliza
			
			printk("\n{ ");
			for(i=0; i<n+1; i++) {
				printk("%c ", rx[i]);
				rxChar(rx[i]);					// Envia para código ser processado
			}
			printk("}\n");

			err = cmdProcessor(&database);		// Função de processamento 
			printk("%d\n", err);
			if (err == 1) {
				print_tx();
			}

			k_mutex_unlock(&test_mutex);			// unlock mutex

			memset(rx, 0, sizeof(rx));			// Mete a 0 o buffer de processamento
			n=0;								

					
		}
		end_thread:

		memset(rx_buf, 0, sizeof(rx_buf));		// Mete a 0 o buffer da uart
		k_busy_wait(1000);
    }
}

K_THREAD_STACK_DEFINE(thread_stack_0, STACKSIZE);
struct k_thread thread_data_0;

K_THREAD_STACK_DEFINE(thread_stack_1, STACKSIZE);
struct k_thread thread_data_1;


int main(void){
	if(!initHardware()){
        printk("[NCS] Error initilizing Hardware\n");
    }

	// Configurar os LEDs como saída para thread0
	gpio_pin_configure_dt(&led_1, GPIO_OUTPUT);
	gpio_pin_configure_dt(&led_2, GPIO_OUTPUT);
	gpio_pin_configure_dt(&led_3, GPIO_OUTPUT);
	gpio_pin_configure_dt(&led_4, GPIO_OUTPUT);

	k_thread_create(&thread_data_0, thread_stack_0, STACKSIZE,
                    thread0, NULL, NULL, NULL,
                    THREAD0_PRIORITY, 0, K_NO_WAIT);

	k_thread_create(&thread_data_1, thread_stack_1, STACKSIZE,
                    thread1, NULL, NULL, NULL,
                    THREAD0_PRIORITY, 0, K_NO_WAIT);
	return 0;
}

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
	gpio_pin_set_dt(&led_1, 1);
	gpio_pin_set_dt(&led_2, 0);
	gpio_pin_set_dt(&led_3, 1);
	gpio_pin_set_dt(&led_4, 0);


    // Check if UART is ready
    if(!device_is_ready(uart)){
        printk("[NCS] Error: UART device not ready\n");
    }
    printk("[NCS] Device UART ready\n");
	returnValue = uart_callback_set(uart, uart_cb, NULL);
	if(returnValue){
		return 1;
	}
	returnValue = uart_tx(uart, tx_buf, sizeof(tx_buf), SYS_FOREVER_MS);
	if(returnValue){
		return 1;
	}
	returnValue = uart_rx_enable(uart, rx_buf, sizeof(rx_buf), RECEIVE_TIMEOUT);
	if(returnValue){
		return 1;
	}

	// Check if ADC is ready
	if (!device_is_ready(adc_dev)) {
		printk("[NCS] Error: UART device not ready\n");
	}
	int err = adc_channel_setup(adc_dev, &chl0_cfg);
	if (err != 0) {
		printk("ADC adc_channel_setup failed with error %d.\n", err);
	}
	return 1;
}

void initRTDB(RTDB *rtdb){
    rtdb->led[0] = 0;
    rtdb->led[1] = 0;
    rtdb->led[2] = 0;
    rtdb->led[3] = 0;
    rtdb->but[0] = 0;
    rtdb->but[1] = 0;
    rtdb->but[2] = 0;
    rtdb->but[3] = 0;
    rtdb->anRaw = 0;
    rtdb->anVal = 0;
}


