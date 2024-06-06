/** @file funcs.h
 * @brief Definition of some auiliary functions and the Real Time Database structure
 * 
 * @author Gonçalo Peralta & João Alvares
 * @date 03 June 2024
 * @bug No known bugs.
*/
#ifndef FUNCS_H
#define FUNCS_H

/**
 * @brief Real-time database
 * 
 * This database holds the LED and button states and the analog reader value.
*/
typedef struct{
    int led[4];    /**< LEDs 1 to 4 state (1 for ON and 0 for OFF)*/
    int but[4];    /**< Button 1 to 4 state (1 for pressed and 0 for not pressed)*/
    int anRaw;     /**< Raw value of the analog reader (0 to 1024 assuming 10 bits)*/
    float anVal;   /**< Converted value of the analog reader (0V to 3V)*/
} RTDB;

/**
 * @brief Initilizes the database with zeros
 * 
 * @param[in] rtdb pointer to the RTDB
 * @return void 
*/
void initRTDB(RTDB *rtdb);

/**
 * @brief Updates the frequecy of update of the RTDB
 * 
 * @param[in] x period time in seconds
 * @return void
 */
void updateFreq(int x);

/**
 * @brief Initializes the Hardware needed for the program
 * 
 * Initializes the 4 LEDs, the 4 Buttons, the UART device and the ADC device
 * 
 * @return void
 */
int initHardware();

#endif
