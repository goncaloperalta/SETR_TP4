#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cmd.h"

static unsigned char UARTRxBuffer[UART_RX_SIZE];
static unsigned char rxBufLen = 0; 
static unsigned char UARTTxBuffer[UART_TX_SIZE];
static unsigned char txBufLen = 0;

int cmdProcessor(void){
	int i, sizeCMD = 0, err = 0;
	unsigned char type = 0; // Sensor type
	unsigned char expectedChecksum = 0;
	char receivedChecksum[4], checksum[4];
	char txCmd[UART_TX_SIZE], data_temp[4], data_hum[4], data_co2[6];

	if(rxBufLen == 0){
		return EMPTY_BUFFER;
	}
		
	for(i = 0; i < rxBufLen; i++){
		if(UARTRxBuffer[i] == SOF_SYM){
			break;
		}
	}
	
	if(i < rxBufLen){
		switch(UARTRxBuffer[i+1]){
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
				
				// Create a response command
				
				txCmd[0] = '#'; 		 // SOF
				txCmd[1] = 'a';  		 // a
				
                sprintf(checksum, "%03d", calcChecksum((unsigned char*)&(txCmd[1]), 12));
				txCmd[13] = checksum[0]; // CS
				txCmd[14] = checksum[1]; // CS
				txCmd[15] = checksum[2]; // CS
				txCmd[16] = '!'; // EOF
				sizeCMD = 17;

				// Send through the Transmiter buffer
				clear_tx_cmd();
				for(int j = 0; j < sizeCMD; j++){
					txChar(txCmd[j]);
				}
				
				clear_rx_cmd();
				return SUCCESS;
	
			case 'W': // #P[t/h/c][CS]!
				// Validate frame structure
				type = UARTRxBuffer[i+2];
				if(type != 't' && type != 'h' && type != 'c'){ // #P[t/h],,,!
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
				
				// Create a response command
				if(type == 'c'){
					if((err = getSensorReading(data_co2, type)) == BAD_PARAMETER){
						clear_rx_cmd();
						return err;
					}
				} else{
					if((err = getSensorReading(data_temp, type)) == BAD_PARAMETER){
						clear_rx_cmd();
						return err;
					}
				}

				txCmd[0] = '#';  // SOF
				txCmd[1] = 'p';  // p
				txCmd[2] = type; // sensor type (same has the one received)
				if(type != 'c'){
					txCmd[3] = data_temp[0]; // DATA 
					txCmd[4] = data_temp[1]; // DATA
					txCmd[5] = data_temp[2]; // DATA
					sprintf(checksum, "%03d", calcChecksum((unsigned char*)&(txCmd[1]), 5));
					txCmd[6] = checksum[0]; // CS
					txCmd[7] = checksum[1]; // CS
					txCmd[8] = checksum[2];	// CS
					txCmd[9] = '!'; // EOF
					sizeCMD = 10;
				} else{ // Co2 needs 5 digits for the data
					txCmd[3] = data_co2[0]; // DATA 
					txCmd[4] = data_co2[1]; // DATA
					txCmd[5] = data_co2[2]; // DATA
					txCmd[6] = data_co2[3]; // DATA
					txCmd[7] = data_co2[4]; // DATA
					sprintf(checksum, "%03d", calcChecksum((unsigned char*)&(txCmd[1]), 7));
					txCmd[8] = checksum[0]; // CS
					txCmd[9] = checksum[1]; // CS
					txCmd[10] = checksum[2]; // CS
					txCmd[11] = '!'; // EOF
					sizeCMD = 12;
				}

				// Send through the Transmiter buffer
				clear_tx_cmd();
				for(int j = 0; j < sizeCMD; j++){
					txChar(txCmd[j]);
				}

				clear_rx_cmd();
				return SUCCESS;
			case 'L': // #L[CS]!
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

				printf("\n---- Printing variables History ----\n");
				printf("\n");
				clear_rx_cmd();
				return SUCCESS;
			case 'R': // #R[CS]!
				// Validate frame structure
				if(UARTRxBuffer[i+5] != EOF_SYM){
					resetRxBuffer();
					return MISSING_EOF;
				}
				// Validate Checksum
				receivedChecksum[0] = UARTRxBuffer[i+2];
				receivedChecksum[1] = UARTRxBuffer[i+3];
				receivedChecksum[2] = UARTRxBuffer[i+4];
				expectedChecksum = calcChecksum(&(UARTRxBuffer[i+1]), 1);
				if(atoi(receivedChecksum) != expectedChecksum){
					clear_rx_cmd();
					return WRONG_CS;
				}

				clear_rx_cmd();
				return SUCCESS;
			default:
				clear_rx_cmd();
				return UNKNOWN_CMD;				
		}
	}
	resetRxBuffer();
	return MISSING_SOF;
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
	printf("\n ------ Printing RX Buffer ------ \n {");
	for(int i = 0; i < UART_RX_SIZE; i++){
		printf("'%c',", UARTRxBuffer[i]);
	}
	printf("}\nBuffer length: %d\n", rxBufLen);
}

void print_tx(){
	printf("\n ------ Printing TX Buffer ------ \n {");
	for(int i = 0; i < UART_TX_SIZE; i++){
		printf("'%c',", UARTTxBuffer[i]);
	}
	printf("}\nBuffer length: %d\n\n", txBufLen);
}