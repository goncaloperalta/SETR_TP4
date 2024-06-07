/** @file cmdproc.h
 * @brief Definition of the function for the Command Processor module
 * 
 * @author Gonçalo Peralta & João Alvares
 * @date 03 June 2024
 * @bug No known bugs.
*/

// #include <zephyr/kernel.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/gpio.h>
// #include <zephyr/sys/util.h>
// #include <zephyr/sys/printk.h>
// #include <zephyr/drivers/uart.h>
// #include <zephyr/devicetree.h>
// #include <zephyr/drivers/adc.h>

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cmdproc.h"
#include "../../includes/funcs.h"

// extern struct k_mutex test_mutex; // Mutex from main file

int cmdProcessor(char *cmd, char *resp, RTDB *database){
	unsigned char expectedChecksum = 0;
	char receivedChecksum[4], checksum[4];
	int var = 0;

	if(cmd[0] == SOF_SYM){
		switch(cmd[1]){
			case 'B': // # B [CS] ! - Read buttons state, resp: # b [0/1][0/1][0/1][0/1] [CS] !
				// Validate frame structure
				if(cmd[5] != EOF_SYM){
					return MISSING_EOF;
				}
				
				// Validate checksum
				expectedChecksum = calcChecksum(&(cmd[1]), 1);
				sprintf(receivedChecksum, "%c%c%c", cmd[2], cmd[3], cmd[4]);
				if(atoi(receivedChecksum) != expectedChecksum){
					return WRONG_CS;
				}
				
				// Response Command
				// k_mutex_lock(&test_mutex, K_FOREVER); 	// A Reading of the RTDB is about to begin lets lock the access
				
				sprintf(resp, "b%d%d%d%d", database->but[0], database->but[1], database->but[2], database->but[3]);
				sprintf(checksum, "%03d", calcChecksum((unsigned char*)&(resp[0]), 5));
				sprintf(resp, "#b%d%d%d%d%s!", database->but[0], database->but[1], database->but[2], database->but[3], checksum);
				
				// k_mutex_unlock(&test_mutex);			// Reading done, time to unlock
				
				return SUCCESS;	// case 'B'
			case 'L': // # L [1/2/3/4] [CS] ! - Toggle LED state, resp = # l [1/2/3/4] [0/1] [CS] !
				// Validate frame structure
				if(cmd[6] != EOF_SYM){
					return MISSING_EOF;
				}

				// Validate LED number (1 to 4)
				if(cmd[2] < '1' || cmd[2] > '4'){
					return UNKNOWN_LED;
				}
				
				// Validate checksum
				expectedChecksum = calcChecksum(&(cmd[1]), 2);
				sprintf(receivedChecksum, "%c%c%c", cmd[3], cmd[4], cmd[5]);
				if(atoi(receivedChecksum) != expectedChecksum){
					return WRONG_CS;
				}

				// Toggle LED
				// k_mutex_lock(&test_mutex, K_FOREVER);

				database->led[cmd[2]-1-'0'] = database->led[cmd[2]-1-'0'] == 1 ? 0 : 1;
				var = database->led[cmd[2]-1-'0'];
				
				// k_mutex_unlock(&test_mutex);

				// Response Command
				sprintf(resp, "l%c%d", cmd[2], var);
				sprintf(checksum, "%03d", calcChecksum((unsigned char*)&(resp[0]), 3));
				sprintf(resp, "#l%c%d%s!", cmd[2], var, checksum);

				return SUCCESS;	// case 'L'
			case 'A': // # A [CS] ! - Read Analog sensor, resp = # a [1-9][1-9][1-9][1-9] [CS] !
				// Validate frame structure
				if(cmd[5] != EOF_SYM){
					return MISSING_EOF;
				}
				
				// Validate checksum
				expectedChecksum = calcChecksum(&(cmd[1]), 1);
				sprintf(receivedChecksum, "%c%c%c", cmd[2], cmd[3], cmd[4]);
				if(atoi(receivedChecksum) != expectedChecksum){
					return WRONG_CS;
				}

				// Read AnVal
				// k_mutex_lock(&test_mutex, K_FOREVER);
				
				var = (int)database->anRaw;

				// k_mutex_unlock(&test_mutex);

				// Response Command
				sprintf(resp, "a%04d", var);
				sprintf(checksum, "%03d", calcChecksum((unsigned char*)&(resp[0]), 5));
				sprintf(resp, "#a%04d%s!", var, checksum);

				return SUCCESS;	// case 'A'
			case 'U': // # U [1-9][1-9] [CS] ! - Change frequecy of update of the in/out digital signals of RTDB, resp = # u [1-9][1-9] [CS] !
				// Validate frame structure
				if(cmd[7] != EOF_SYM){
					return MISSING_EOF;
				}
				
				// Validate Frequecy value
				if(!isdigit(cmd[2]) || !isdigit(cmd[2])){
					return INVALID_FREQ;
				}

				// Validate checksum
				expectedChecksum = calcChecksum(&(cmd[1]), 3);
				sprintf(receivedChecksum, "%c%c%c", cmd[4], cmd[5], cmd[6]);
				if(atoi(receivedChecksum) != expectedChecksum){
					return WRONG_CS;
				}

				// Response Command
				var = ((cmd[2] - '0')*10 + (cmd[3] - '0'))*pow(10, 6); // New frequecy of update
				sprintf(resp, "u%c%c", cmd[2], cmd[3]);
				sprintf(checksum, "%03d", calcChecksum((unsigned char*)&(resp[0]), 3));
				sprintf(resp, "#u%c%c%s!", cmd[2], cmd[3], checksum);

				return SUCCESS;	// case 'U'
			default:
				return UNKNOWN_CMD;				
		}
	}
	return MISSING_SOF;
}

unsigned char calcChecksum(unsigned char *buf, int nbytes){
	unsigned char checksum = 0;
	for(int i = 0; i < nbytes; i++){ // computing expected checksum
		checksum += buf[i];
	}

	return checksum;
}