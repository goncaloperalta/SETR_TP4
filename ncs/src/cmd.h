/** @file cmd.h
 * @brief Yields the functions and the structures of the program. Based on the provided base code
 *
 * This file has provides a set of functions to process a command in the UART.
 * 
 * @author Gonçalo Peralta & João Alvares
 * @date 30 March 2024
 * @bug No known bugs.
 */

#ifndef CMD_H_
#define CMD_H_

#include "main.h"

#define UART_RX_SIZE 20 	/**< Maximum size of the RX buffer */ 
#define UART_TX_SIZE 20 	/**< Maximum size of the TX buffer */ 
#define SOF_SYM '#'	        /**< Start of Frame Symbol */
#define EOF_SYM '!'         /**< End of Frame Symbol */
#define HIST_SIZE 20        /**< Maximum size for the History variables */

#define SUCCESS 1                   /**< Operation completed without errors */
#define MISSING_SOF -100            /**< Missing start of frame caracter '#' */
#define MISSING_EOF -101            /**< Missing end of frame caracter '!' */
#define MISSING_SENSOR_TYPE -102    /**< Missing sensor type for 'P' command */
#define EMPTY_BUFFER -103           /**< Rx Buffer is empty */
#define WRONG_CS -104               /**< Expected checksum is not equal to the received one */
#define RX_FULL -105                /**< Rx Buffer is full */
#define TX_FULL -106                /**< Tx Buffer is full */
#define UNKNOWN_CMD -107            /**< Command not identified */
#define BAD_PARAMETER -108          /**< Reading out of bounds */
#define OUT_OF_READING -109         /**< No more readings available*/

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
int cmdProcessor(RTDB *database);

/**
 * @brief Adds a character to the Rx Buffer
 * 
 * @param[in] car character to be added
 * @return RX_FULL if Rx Buffer is full, SUCCESS if no error occours 
*/
int rxChar(unsigned char car);

/**
 * @brief Adds a character to the Tx Buffer
 * 
 * @param[in] car character to be added
 * @return TX_FULL if Tx Buffer is full, SUCCESS if no error occours 
*/
int txChar(unsigned char car);

/**
 * @brief Resets the Rx Buffer
 * 
 * Calling this function sets the rxBufLen to zero
 * @return void
*/
void resetRxBuffer(void);

/**
 * @brief Resets the Tx Buffer
 * 
 * Calling this function sets the txBufLen to zero
 * @return void
*/
void resetTxBuffer(void);

/**
 * @brief Returns the data on the Tx Buffer and length
 * 
 * Sets the contents of buf to the same as the Tx Buffer and len to the length of the buffer
 * @param[in] buf buffer to copy the Tx Buffer contents
 * @param[in] len buffer variable that gets set to the length of the Tx Buffer
 * @return void
*/
void getTxBuffer(unsigned char * buf, int * len);

/**
 * @brief Returns the data on the Rx Buffer and length
 * 
 * Sets the contents of buf to the same as the Rx Buffer and len to the length of the buffer
 * @param[in] buf buffer to copy the Rx Buffer contents
 * @param[in] len buffer variable that gets set to the length of the Rx Buffer
 * @return void
*/
void getRxBuffer(unsigned char * buf, int * len);

/**
 * @brief Computes the modulo 256 checksum of a given number of bytes
 * 
 * Calculates the modulo 256 sum in ASCII of the first nbytes of the provided buffer. Returns the value. 
 * @param[in] buf buffer with the caracters to calculate the checksum
 * @param[in] nbytes number of bytes to get into account on the sum
 * @return unsigned char with the value of the checksum
*/
unsigned char calcChecksum(unsigned char * buf, int nbytes);

/**
 * @brief Clears the first command in the Rx Buffer
 * 
 * Looks for the first command in the Rx Buffer and clears it. 
 * @return EMPTY_BUFFER if buffer len is zero, SUCCESS if no error
*/
int clear_rx_cmd();

/**
 * @brief Clears the first command in the Tx Buffer
 * 
 * Looks for the first command in the Tx Buffer and clears it. 
 * @return EMPTY_BUFFER if buffer len is zero, SUCCESS if no error
*/
int clear_tx_cmd();

/**
 * @brief Prints the contens of the Rx Buffer
 * 
 * Prints to the terminal all bytes in the Rx Buffer.
 * @return void
*/
void print_rx();

/**
 * @brief Prints the contens of the Tx Buffer
 * 
 * Prints to the terminal all bytes in the Tx Buffer.
 * @return void
*/
void print_tx();

#endif
