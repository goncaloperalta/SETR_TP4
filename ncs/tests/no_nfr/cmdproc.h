/** @file cmdproc.h
 * @brief Yields the functions and the structures of the program. Based on the provided base code
 *
 * This file has provides a set of functions to process a command in the UART.
 * 
 * @author Gonçalo Peralta & João Alvares
 * @date 06 June 2024
 * @bug No known bugs.
 */

#ifndef CMD_PROC_H_
#define CMD_PROC_H_

#define UART_RX_SIZE 20 	/**< Maximum size of the RX buffer */ 
#define UART_TX_SIZE 20 	/**< Maximum size of the TX buffer */ 
#define SOF_SYM '#'	        /**< Start of Frame Symbol */
#define EOF_SYM '!'         /**< End of Frame Symbol */
#define HIST_SIZE 20        /**< Maximum size for the History variables */

#define SUCCESS 1           /**< Operation completed without errors */
#define MISSING_SOF -100    /**< Missing start of frame caracter '#' */
#define MISSING_EOF -101    /**< Missing end of frame caracter '!' */
#define WRONG_CS -102       /**< Expected checksum is not equal to the received one */
#define UNKNOWN_CMD -103    /**< Command not identified */
#define UNKNOWN_LED -104    /**< LED number not identified*/
#define INVALID_FREQ -105   /**< Provided frequency is not valid*/

#include "../../includes/funcs.h"

/**
 * @brief Processes the characters in the Rx buffer looking for a command
 * 
 * The structure of a command in the Rx Buffer is: "# CMD CS !" (without the spaces) <br>
 * <ul>
 *      <li> # &rarr; Start of frame <br>
 *      <li> CMD &rarr; Type of command 'B'/'L'/'A'/'U', more information bellow <br>
 *      <li> CS &rarr; Modulo 256 3-bit checksum of the bytes in the CMD <br>
 *      <li> ! &rarr; End of frame <br>
 * </ul>     
 * Depending on the provided CMD a reponse command is sent to the Tx Buffer <br>
 * Types of CMD: <br>
 * <ul>
 *       <li> 'B' &rarr; Reads the state of all Buttons (1-4). A command is sent to the Tx Buffer with structure "# CMD DATA CS !" where: <br>
 *       <ul>
 *          <li> CMD &rarr; in this case will be the byte 'b' <br>
 *          <li> DATA &rarr; containts 4 bytes of type '1' (pressed) or '0' (not pressed) for each button 1 to 4 <br>
 *          <li> CS &rarr; checksum of CMD and DATA bytes <br>
 *       </ul>
 *       <li> 'L','[1/2/3/4]' &rarr; Toggles the state of the provided LED number. A command is sent to the Tx Buffer with structure "# CMD CS !" where: <br>
 *       <ul>
 *          <li> CMD &rarr; 'l' <br>
 *       </ul>
 *       <li> 'A' &rarr; Reads the analog sensor. A command is sent to the Tx Buffer with structure "# CMD DATA CS !" where: <br>
 *       <ul>
 *          <li> CMD &rarr; 'a' <br>
 *          <li> DATA &rarr; 3 bytes corresponding to the value read <br>
 *       </ul>
 *       <li> 'U','[x/x]' &rarr; Change frequecy of update of the in/out digital signals of RTDB to xx MHz. A command is sent to the Tx Buffer with structure "# CMD CS !" where: <br>
 *       <ul>
 *          <li> CMD &rarr; 'u' <br>
 *          <li> DATA &rarr; 'xx' (same as the provided one) <br>
 *       </ul>
 *  </ul>
 * @return EMPTY_BUFFER if Rx Buffer is empty, MISSING_EOF if '!' is not found, WRONG_CS if checksum is wrong, MISSING_SENSOR_TYPE sensor type is not found (for 'P' CMD), MISSING_SOF if a '#' is not found and UNKNOWN_CMD if the CMD is not identified
 */
int cmdProcessor(char *cmd, char *resp, RTDB *database);

/**
 * @brief Computes the modulo 256 checksum of a given number of bytes
 * 
 * Calculates the modulo 256 sum in ASCII of the first nbytes of the provided buffer. Returns the value. 
 * @param[in] buf buffer with the caracters to calculate the checksum
 * @param[in] nbytes number of bytes to get into account on the sum
 * @return unsigned char with the value of the checksum
*/
unsigned char calcChecksum(unsigned char * buf, int nbytes);

#endif