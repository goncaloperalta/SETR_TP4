
/** @file cmdproc.h
 * @brief Yields the functions and the structures of the program. Based on the provided base code
 *
 * This file has provides a set of functions to emulate a UART communication with sensors.
 * 
 * Buffer circular no historico
 * ACK 
 * @author Gonçalo Peralta & João Alvares
 * @date 30 March 2024
 * @bug No known bugs.
 */

#ifndef CMD_H_
#define CMD_H_

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
 *      <li> CMD &rarr; Type of command 'A'/'P'/'L'/'R', more information bellow <br>
 *      <li> CS &rarr; Modulo 256 3-bit checksum of the bytes in the CMD <br>
 *      <li> ! &rarr; End of frame <br>
 * </ul>     
 * Depending on the provided CMD a reponse command is sent to the Tx Buffer <br>
 * Types of CMD: <br>
 * <ul>
 *       <li> 'A' &rarr; Asks the value of all sensor readings. A command is sent to the Tx Buffer with structure "# CMD DATA CS !" where: <br>
 *       <ul>
 *          <li> CMD &rarr; in this case will be the byte 'a' <br>
 *          <li> DATA &rarr; containts 11 bytes. first 3 correspond to temperature, next 3 to humidity and last 5 to co2. see getSensorReading() for more info <br>
 *          <li> CS &rarr; checksum of CMD and DATA bytes <br>
 *       </ul>
 *       <li> 'P','[t/h/c]' &rarr; Asks the value of a sepecific sensor designed by the second byte ('t'/'h'/'c'). A command is sent to the Tx Buffer with structure "# CMD DATA CS !" where: <br>
 *       <ul>
 *          <li> CMD &rarr; 'p','[t/h/c]' <br>
 *          <li> DATA &rarr; if temperature 3 bytes, humidity 3 bytes and co2 5 bytes <br>
 *       </ul>
 *       <li> 'L' &rarr; Prints to the terminal the last 20 samples read of each sensor <br>
 *       <li> 'R' &rarr; Resets the history <br>
 * </ul>
 * @return EMPTY_BUFFER if Rx Buffer is empty, MISSING_EOF if '!' is not found, WRONG_CS if checksum is wrong, MISSING_SENSOR_TYPE sensor type is not found (for 'P' CMD), MISSING_SOF if a '#' is not found and UNKNOWN_CMD if the CMD is not identified
 */
int cmdProcessor(void);

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

/**
 * @brief Changes the contens of buf to a sensor reading in string form
 * 
 * Gets a sensor reading, converts it to a string of digits and sets it to the content of buf. <br>
 * There are 3 types of sensors temperature, humidity and co2: <br>
 * Temperature [-50, 60]ºC: <br>
 * <ul>
 *      <li> buf[0] &rarr; it's set to '+' or '-' <br> 
 *      <li> buf[1] &rarr; it's set most significant digit, if number is less than 10 gets set to '0' <br>
 *      <li> buf[2] &rarr; it's set less significant digit <br>
 *      <li> Example: 7 &rarr; '+','0','7' <br>
 * </ul>
 * Humidity [0, 100]%: <br>
 * <ul>
 *      <li> buf[0] &rarr; it's set most significant digit, if number is less than 100 gets set to '0' <br>
 *      <li> buf[1] &rarr; it's set second most significant digit, if number is less than 10 gets set to '0' <br>
 *      <li> buf[2] &rarr; it's set less significant digit <br>
 *      <li> Example: 7 &rarr; '0','0','7' <br>
 * </ul>
 * Co2 [400, 20000]ppm: <br>
 * <ul>
 *      <li> buf[0] &rarr; it's set most significant digit, if number is less than 10000 gets set to '0' <br>
 *      <li> buf[1] &rarr; it's set second most significant digit, if number is less than 1000 gets set to '0' <br>
 *      <li> buf[2] &rarr; it's set third most significant digit <br>
 *      <li> buf[3] &rarr; it's set forth most significant digit <br>
 *      <li> buf[4] &rarr; it's set less significant digit <br>
 *      <li> Example: 458 &rarr; '0','0','4','5','8' <br>
 * </ul>
 * @param[in] buf buffer to store the digits
 * @param[in] type type of sensor: 't' for temperature, 'h' for humidity and 'c' for co2
 * @return BAD_PARAMETER if reading is out of bounds, MISSING_SENSOR_TYPE if type is not 't'/'h'/'c', SUCCESS if no errors occour
*/
int getSensorReading(char *buf, char type);

#endif
