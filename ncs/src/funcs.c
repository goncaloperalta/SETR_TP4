/** @file funcs.c
 * 
 * @brief Implementation of some basic auxiliary functions
 * 
 * @author Gonçalo Peralta & João Alvares
 * @date 03 June 2024
 * @bug No known bugs.
 */
#include "../includes/funcs.h"

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

void consoleLog(int err){
    char errorLog[6][50] = {"Missing Start of frame '#'", "Missing Eof of frame '!'", "Wrong Checksum", "Invalid type not identified", "Invalid LED number", "Invalid Frequency"};
	printk("[LOG] Error in command structure: %s\n", errorLog[abs(err)-100]);
}
