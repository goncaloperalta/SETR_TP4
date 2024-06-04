#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/uart.h>

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../includes/cmd.h"
#include "../includes/main.h"

extern const struct gpio_dt_spec button_1;
extern const struct gpio_dt_spec button_2;
extern const struct gpio_dt_spec button_3;
extern const struct gpio_dt_spec button_4;

extern const struct gpio_dt_spec led_1;
extern const struct gpio_dt_spec led_2;
extern const struct gpio_dt_spec led_3;
extern const struct gpio_dt_spec led_4;

extern int timer;

static unsigned char UARTRxBuffer[UART_RX_SIZE];
static unsigned char rxBufLen = 0; 
static unsigned char UARTTxBuffer[UART_TX_SIZE];
static unsigned char txBufLen = 0;

int cmdProcessor(RTDB *database){
	int i, sizeCMD = 0;
	unsigned char type = 0; // Led number
	unsigned char expectedChecksum = 0;
	char receivedChecksum[4], checksum[4];
	char txCmd[UART_TX_SIZE];
	unsigned char val[4];

	if(rxBufLen == 0){
		return EMPTY_BUFFER;
	}
		
	i = 0;
	if(UARTRxBuffer[i] != SOF_SYM){
		return MISSING_SOF;
	}
	
	
	if(i < rxBufLen){
		switch(UARTRxBuffer[i+1]) {
            // Commands
            // # B [CS] ! 					- Read button state
            // # L [1/2/3/4] [1/0] [CS] ! 	- Set LED state
            // # A [CS] ! 					- Read Analog sensor
            // # U [0000] [CS] !	 		- Change frequecy of update of the in/out digital signals of RTDB
            // # S [0000] [CS] ! 			- Change frequecy of sampling of analog input signal
            // # P [CS] ! 					- Toggle Analog reading mode
			case 'B': // #A[CS]!
				// Validate frame structure
				if(UARTRxBuffer[i+5] != EOF_SYM){
					resetRxBuffer();
					return MISSING_EOF;
				}
				
				// Validate checksum
				expectedChecksum = calcChecksum(&(UARTRxBuffer[i+1]), 1);
				receivedChecksum[0] = UARTRxBuffer[i+2];
				receivedChecksum[1] = UARTRxBuffer[i+3];
				receivedChecksum[2] = UARTRxBuffer[i+4];
				if(atoi(receivedChecksum) != expectedChecksum){
					clear_rx_cmd();
					return WRONG_CS;
				}

				if (database->but[0]) {

				}
				// Create a response command
				txCmd[0] = '#'; 		 // SOF
				txCmd[1] = 'b';  		 // b
				txCmd[2] = database->but[0] ? '1' : '0';
				txCmd[3] = database->but[1] ? '1' : '0';
				txCmd[4] = database->but[2] ? '1' : '0';
				txCmd[5] = database->but[3] ? '1' : '0';
                sprintf(checksum, "%03d", calcChecksum((unsigned char*)&(txCmd[1]), 5));
				txCmd[6] = checksum[0]; // CS
				txCmd[7] = checksum[1]; // CS
				txCmd[8] = checksum[2]; // CS
				txCmd[9] = '!'; // EOF
				sizeCMD = 10;

				// Send through the Transmiter buffer
				clear_tx_cmd();
				for(int j = 0; j < sizeCMD; j++){
					txChar(txCmd[j]);
				}
				
				clear_rx_cmd();
				return SUCCESS;
	
			case 'L': // #L[1/2/3/4][CS]!
				// Validate frame structure
				type = UARTRxBuffer[i+2];
				int numero = type - '0';
				if(numero<1 || numero>4){ // #L[1/2/3/4],,,!
					clear_rx_cmd();
					return MISSING_SENSOR_TYPE;
				}
				if(UARTRxBuffer[i+6] != EOF_SYM){
					resetRxBuffer();
					return MISSING_EOF;
				}

				// Validate Checksum
				receivedChecksum[0] = UARTRxBuffer[i+3];
				receivedChecksum[1] = UARTRxBuffer[i+4];
				receivedChecksum[2] = UARTRxBuffer[i+5];
				expectedChecksum = atoi(receivedChecksum);
				if(expectedChecksum != calcChecksum(&(UARTRxBuffer[i+1]), 2)){
					clear_rx_cmd();
					return WRONG_CS;
				}
				
				// Toggle Led
				if (numero == 1) {
					database->led[0] = !database->led[0];
				}
				else if (numero == 2) {
					database->led[1] = !database->led[1];
				}
				else if (numero == 3) {
					database->led[2] = !database->led[2];
				}
				else {
					database->led[3] = !database->led[3];
				}

				// Create a response command
				txCmd[0] = '#';  // SOF
				txCmd[1] = 'l';  // p
				sprintf(checksum, "%03d", calcChecksum((unsigned char*)&(txCmd[1]), 1));
				txCmd[2] = checksum[0]; // CS
				txCmd[3] = checksum[1]; // CS
				txCmd[4] = checksum[2]; // CS
				txCmd[5] = '!'; // EOF
				sizeCMD = 6;

				// Send through the Transmiter buffer
				clear_tx_cmd();
				for(int j = 0; j < sizeCMD; j++){
					txChar(txCmd[j]);
				}

				clear_rx_cmd();
				return SUCCESS;

			case 'A': // #A[CS]!
				// Validate frame structure
				if(UARTRxBuffer[i+5] != EOF_SYM){
					resetRxBuffer();
					return MISSING_EOF;
				}
				receivedChecksum[0] = UARTRxBuffer[i+2];
				receivedChecksum[1] = UARTRxBuffer[i+3];
				receivedChecksum[2] = UARTRxBuffer[i+4];
				expectedChecksum = calcChecksum(&(UARTRxBuffer[i+1]), 1);
				if(atoi(receivedChecksum) != expectedChecksum){
					clear_rx_cmd();
					return WRONG_CS;
				}
				
				// Valor anVal da ADC
				if (database->anVal >= 100) {
        			val[0] = (database->anVal / 100) + '0';
        			database->anVal %= 100;
    			}
				else {
					val[0] = '0';
				}
    			if (database->anVal >= 10) {
        			val[1] = (database->anVal / 10) + '0';
        			database->anVal %= 10;
    			}
				else {
					val[1] = '1';
				}
    			val[2] = database->anVal + '0';

				// Create a response command
				txCmd[0] = '#';  // SOF
				txCmd[1] = 'a';  // a
				txCmd[2] = val[0];
				txCmd[3] = val[1];
				txCmd[4] = val[2];
				sprintf(checksum, "%03d", calcChecksum((unsigned char*)&(txCmd[1]), 4));
				txCmd[5] = checksum[0]; // CS
				txCmd[6] = checksum[1]; // CS
				txCmd[7] = checksum[2]; // CS
				txCmd[8] = '!'; // EOF
				sizeCMD = 9;

				// Send through the Transmiter buffer
				clear_tx_cmd();
				for(int j = 0; j < sizeCMD; j++){
					txChar(txCmd[j]);
				}

				clear_rx_cmd();
				return SUCCESS;
			case 'U': // #U[val][CS]!
				// Validate frame structure
				if(UARTRxBuffer[i+7] != EOF_SYM){
					resetRxBuffer();
					return MISSING_EOF;
				}
				// Validate Checksum
				receivedChecksum[0] = UARTRxBuffer[i+4];
				receivedChecksum[1] = UARTRxBuffer[i+5];
				receivedChecksum[2] = UARTRxBuffer[i+6];
				expectedChecksum = calcChecksum(&(UARTRxBuffer[i+1]), 3);
				if(atoi(receivedChecksum) != expectedChecksum){
					clear_rx_cmd();
					return WRONG_CS;
				}
				
				int digito_x = UARTRxBuffer[i+2] - '0';
				int digito_x1 = UARTRxBuffer[i+3] - '0';
				timer = digito_x*10 + digito_x1;

				//printk ("Tempo de espera: %d\n", timer);
				// Create a response command
				txCmd[0] = '#';  // SOF
				txCmd[1] = 'u';  // u
				txCmd[2] = UARTRxBuffer[i+2];
				txCmd[3] = UARTRxBuffer[i+3];
				sprintf(checksum, "%03d", calcChecksum((unsigned char*)&(txCmd[1]), 3));
				txCmd[4] = checksum[0]; // CS
				txCmd[5] = checksum[1]; // CS
				txCmd[6] = checksum[2]; // CS
				txCmd[7] = '!'; // EOF
				sizeCMD = 8;

				// Send through the Transmiter buffer
				clear_tx_cmd();
				for(int j = 0; j < sizeCMD; j++){
					txChar(txCmd[j]);
				}
				clear_rx_cmd();
				return SUCCESS;
			default:
				clear_rx_cmd();
				return UNKNOWN_CMD;				
		}
	}
	resetRxBuffer();
	return SUCCESS;
}

unsigned char calcChecksum(unsigned char *buf, int nbytes){
	unsigned char checksum = 0;
	for(int i = 0; i < nbytes; i++){ // computing expected checksum
		checksum += buf[i];
	}

	return checksum;
}

int rxChar(unsigned char car){
	if(rxBufLen < UART_RX_SIZE){
		UARTRxBuffer[rxBufLen] = car;
		rxBufLen += 1;
		return SUCCESS;
	}
	return RX_FULL;
}

int txChar(unsigned char car){
	if(txBufLen < UART_TX_SIZE){
		UARTTxBuffer[txBufLen] = car;
		txBufLen += 1;
		return SUCCESS;
	}
	return TX_FULL;
}

void resetRxBuffer(void){
	rxBufLen = 0;
	return;
}

void resetTxBuffer(void){
	txBufLen = 0;
	return;
}

void getTxBuffer(unsigned char *buf, int *len){
	*len = txBufLen;
	if(txBufLen > 0){
		memcpy(buf, UARTTxBuffer, *len);
	}
	return;
}

void getRxBuffer(unsigned char *buf, int *len){
	*len = rxBufLen;
	if(rxBufLen > 0){
		memcpy(buf, UARTRxBuffer, *len);
	}
	return;
}

int clear_rx_cmd(){
	if(rxBufLen == 0){
		return EMPTY_BUFFER;
	}
	int i = 0;
	while(UARTRxBuffer[i] != '#'){
		i++;
	}
	int j = i;
	while(UARTRxBuffer[j] != '!'){
		j++;
	}
	for(int k = i; k <= j; k++){
		UARTRxBuffer[k] = '\0';
	}
	rxBufLen = i;

	return SUCCESS;
}

int clear_tx_cmd(){
	if(txBufLen == 0){
		return EMPTY_BUFFER;
	}
	int i = 0;
	while(UARTTxBuffer[i] != '#'){
		i++;
	}
	int j = i;
	while(UARTTxBuffer[j] != '!'){
		j++;
	}
	for(int k = i; k <= j; k++){
		UARTTxBuffer[k] = '\0';
	}
	txBufLen = i;

	return SUCCESS;
}

void print_rx(){
	printk("\n ------ Code ------ \n {");
	for(int i = 0; UARTRxBuffer[i] != '\0'; i++){
		printk("'%c',", UARTRxBuffer[i]);
	}
	memset(UARTRxBuffer, 0, sizeof(UARTRxBuffer));
}

void print_tx(){
	printk("\n ------ Response ------ \n");
	printk("{");

	for(int i = 0; UARTTxBuffer[i] != '\0'; i++){
		printk("'%c'", UARTTxBuffer[i]);
		if (UARTTxBuffer[i] != '!') {
			printk(",");
		}
	}

	printk("}\n");
}
