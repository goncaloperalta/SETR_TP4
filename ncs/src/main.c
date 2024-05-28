#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/uart.h>

#define STACKSIZE 1024
#define RECEIVE_BUFF_SIZE 20
#define RECEIVE_TIMEOUT 100

#define THREAD0_PRIORITY 6
#define THREAD1_PRIORITY 7


// Buttons 1-4
static const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0});
static const struct gpio_dt_spec button_3 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw2), gpios, {0});
static const struct gpio_dt_spec button_4 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw3), gpios, {0});

// LEDs 1-4
static struct gpio_dt_spec led_1 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});
static struct gpio_dt_spec led_2 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0});
static struct gpio_dt_spec led_3 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led2), gpios, {0});
static struct gpio_dt_spec led_4 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led3), gpios, {0});

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));
static uint8_t tx_buf[] =   {"nRF Connect SDK Fundamentals Course\n\r"
                             "Press 1-3 on your keyboard to toggle LEDS 1-3 on your development kit\n\r"};
static uint8_t rx_buf[RECEIVE_BUFF_SIZE] = {0};

typedef struct{
    int led[4];
    int but[4];
    int anRaw;
    int anVal;
} RTDB;

static RTDB database;
int initHardware();
void initRTDB(RTDB *rtdb);
void setLedButRTDB(RTDB *rtdb, bool *val){
	for(int i = 0; i < 4; i++){
		rtdb->led[i] = val[i];
		rtdb->but[i] = val[i];
	}
}

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data){
	switch(evt->type){
		case UART_RX_RDY:
			for(int i = 0; i < RECEIVE_BUFF_SIZE; i++){
				printk("%c, ", rx_buf[i]);
			}
			break;
		case UART_RX_DISABLED:
			uart_rx_enable(dev ,rx_buf,sizeof rx_buf,RECEIVE_TIMEOUT);
			break;
		default:
			break;
	}
}

void thread0(void){
    if(!initHardware()){
        printk("[NCS] Error initilizing Hardware\n");
    }
    initRTDB(&database);
	bool val[4];

    while(1){
        printk("Thread 0\n");

		val[0] = gpio_pin_get_dt(&button_1);
		gpio_pin_set_dt(&led_1, val[0]);
		val[1] = gpio_pin_get_dt(&button_2);
		gpio_pin_set_dt(&led_2, val[1]);
		val[2] = gpio_pin_get_dt(&button_3);
		gpio_pin_set_dt(&led_3, val[2]);
		val[3] = gpio_pin_get_dt(&button_4);
		gpio_pin_set_dt(&led_4, val[3]);

		setLedButRTDB(&database, val);

        k_busy_wait(1000000);
    }
}

void thread1(void){
	// Commands
	// # B [CS] ! 					- Read button state
	// # L [1/2/3/4] [1/0] [CS] ! 	- Set LED state
	// # A [CS] ! 					- Read Analog sensor
	// # U [0000] [CS] !	 		- Change frequecy of update of the in/out digital signals of RTDB
	// # S [0000] [CS] ! 			- Change frequecy of sampling of analog input signal
	// # P [CS] ! 					- Toggle Analog reading mode

    while(1){
        printk("Thread 1\n");


        k_busy_wait(1000000);
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
	returnValue = uart_rx_enable(uart, rx_buf, sizeof rx_buf, RECEIVE_TIMEOUT);
	if(returnValue){
		return 1;
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