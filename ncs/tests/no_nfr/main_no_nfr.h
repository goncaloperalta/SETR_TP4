/** @file main.h
 * @brief
*/
#ifndef MAIN_NO_NFR_H
#define MAIN_NO_NFR_H

/**
 * @brief Real-time database
 * 
 * This database holds the LED and button state and the analog reader value.
*/
typedef struct{
    int led[4];    /**< LEDs 1 to 4 state (1 for ON and 0 for OFF)*/
    int but[4];    /**< Button 1 to 4 state (1 for pressed and 0 for not pressed)*/
    int anRaw;      /**< Raw value of the analog reader*/
    int anVal;      /**< Converted value of the analog reader*/
} RTDB;

/**
 * @brief Initilizes the database with zeros
 * 
 * @param[in] rtdb pointer to the RTDB
 * @return void 
*/
void initRTDB(RTDB *rtdb);

#endif
