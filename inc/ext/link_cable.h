/**
 * Link Cable Protocol (LCP) - Protocol for data exchange via SEGA Link Cable
 * between two Sega Mega Drive / Sega Genesis consoles connected through the second controller port
 * for SGDK
 *
 * BlodTor 2025
 *
 * *************************************************************************************************
 * *************************************************************************************************
 * Master console function call sequence:
 *
 * 1) Initialize Link Cable Protocol.
 * LCP_init();
 *
 * 2) Prepare data for transmission from master console via Link Cable Protocol.
 * LCP_objectToPacketForSend(transferObject, objectType, objectTypeSizes);
 * LCP_objectToPacketForSend(transferObject, objectType, objectTypeSizes);
 * ...
 * LCP_objectToPacketForSend(transferObject, objectType, objectTypeSizes);
 *
 * 3) Trigger external interrupt on slave console and send/receive data via Link Cable Protocol
 * LCP_masterCycle();
 *
 * 4) Extract received objects from packet transmitted by slave console
 * objectType = LCP_getNextObjectFromReceivePacket(transferObject, objectSizes);
 * objectType = LCP_getNextObjectFromReceivePacket(transferObject, objectSizes);
 * ...
 * objectType = LCP_getNextObjectFromReceivePacket(transferObject, objectSizes);
 * until objectType becomes 0
 *
 * 5) Close Link Cable Protocol if no further interaction with slave console is required.
 * LCP_close();
 *
 * *************************************************************************************************
 * *************************************************************************************************
 * Slave console function call sequence:
 *
 * 1) Initialize Link Cable Protocol
 * LCP_init();
 *
 * 2) Open port to enable slave console to handle external interrupts triggered by
 *    master console. Master console can only call LCP_slaveCycle() - the external
 *    interrupt handler on slave console - after this method is called.
 * LCP_open();
 *
 * 3) Prepare data for transmission from slave console via Link Cable Protocol.
 *    Transmission occurs in LCP_slaveCycle() after receiving data from master console
 *    in this method. LCP_slaveCycle() is called asynchronously by suspending game code
 *    execution on slave console when external interrupt from LCP_masterCycle() method
 *    of master console occurs.
 * LCP_objectToPacketForSend(transferObject, objectType, objectTypeSizes);
 * LCP_objectToPacketForSend(transferObject, objectType, objectTypeSizes);
 * ...
 * LCP_objectToPacketForSend(transferObject, objectType, objectTypeSizes);
 *
 * 4) Extract received objects from packet transmitted by master console
 * objectType = LCP_getNextObjectFromReceivePacket(transferObject, objectSizes);
 * objectType = LCP_getNextObjectFromReceivePacket(transferObject, objectSizes);
 * ...
 * objectType = LCP_getNextObjectFromReceivePacket(transferObject, objectSizes);
 * until objectType becomes 0
 *
 * 5) Close Link Cable Protocol if no further interaction with master console is required.
 *    Slave console will stop responding to external interrupts from master console!
 * LCP_close();
 *
 * *************************************************************************************************
 * *************************************************************************************************
 * IMPORTANT USAGE NOTES:
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 * Object types (objectType) must be defined in YOUR game's main code!
 * See constant declaration examples in 'Super Turbo MEGA Pac-Man 2.1' game code
 * starting with 'OBJECT_TYPE_' prefix
 *
 * Object sizes (objectSizes) in bytes must be defined in YOUR game's main code!
 * The protocol transfers 2 bytes of information per cycle, so there's no point in setting
 * object lengths not multiples of 2 bytes! See constant declaration examples in
 * 'Super Turbo MEGA Pac-Man 2.1' game code ending with '_OBJECT_LENGTH' postfix,
 * as well as the LINK_TYPES_LENGHT array definition example which is passed as objectSizes
 * parameter to LCP_objectToPacketForSend() and LCP_getNextObjectFromReceivePacket() functions
 *
 * Master/Slave console determination implementation must be in YOUR game's code!
 * See initControllerPort2() function example in 'Super Turbo MEGA Pac-Man 2.1' game code
 * which determines which of the two consoles becomes master and which becomes slave
 *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 * For debugging, use the function that returns transmission errors via SEGA Link Cable:
 * LCP_getError()
 *
 */
#ifndef _LINK_CABLE_H_
#define _LINK_CABLE_H_

#include "config.h"

#if (MODULE_LINK_CABLE != 0)

#include "types.h"

/**
 * Constants
 */

// Input/output direction for SEGA MD console port 2 (set individually for each bit)
// Determines the data line operation mode (input or output) and controls the interrupt request mask from external devices.
// Essentially, this is memory address 0xA1000B for data transfer control
// via SEGA Link Cable (CTRL port register CTRL 2 in SEGA documentation)
// Bit 7 (INT) - Interrupt on SYN line signal. 0 = disabled; 1 = enabled.
// Bit 6 (PC6) - Data line "SYN" mode. 0 = input; 1 = output.
// Bits 0-5 (PC0-PC5) - Data line mode. 0 = input; 1 = output.
#define LCP_Ctrl   				(*(vu8*) 0xa1000b)

// State of port 2 on SEGA MD console
// Essentially, this is memory address 0xA10005 for direct data transfer
// via SEGA Link Cable (DATA register of CTRL port 2 in SEGA documentation)
//
// Lower 4 bits (0-3) are used for data transfer.
//
// Bit addressing features between consoles:
// Bit 5 in the current console corresponds to bit 6 in the other console
// for the same memory address 0xA10005. If we change bit 5 on our console,
// bit 6 will change in the other console's LCP_Data variable, and vice versa -
// if bit 5 is changed on the other console, bit 6 will change on our console.
//
// Bits 0-3 at address 0xA10005 will have the same value in both consoles,
// i.e., if we change these bits on one console, they will also change on the other.
// Which console is currently responsible for this is determined by the LCP_Ctrl state -
// i.e., which data line is in output mode (sending data),
// and which is in input mode (receiving data).
#define LCP_Data   				(*(vu8*) 0xa10005)

// Bit 5 is set to 1, others to 0. Primarily used to set bit 5 to 1
// in LCP_Data and LCP_Ctrl to signal the other console that it can
// read or send data via Link Cable.
#define LCP_BIT5				0b00100000

// Bit 6 is set to 1, others to 0. Mainly used to determine
// whether bit 6 is set in LCP_Data, signaling that
// data can be read from the other console or sent to the other console via Link Cable.
#define LCP_BIT6				0b01000000

// Bits 7 and 5 are set to 1, other bits are 0. Used to set these bits in LCP_Ctrl, which indicates
// readiness for data exchange through console port 2 upon external interrupt EX-INT (External Interrupt
// in SEGA documentation), i.e., reading data from the port (data reception) in the external interrupt handler LCP_slaveCycle().
//
// LCP_Ctrl bit values:
// - Interrupt on SYN line signal enabled, bit 7 = 1
// - Data line PC5 in output mode (transmission), bit 5 = 1
// - Data lines PC6, PC4, PC3, PC2, PC1, PC0 in input mode (reception). Corresponding bits = 0
//
// At the "hardware" level for my code this means:
// - Enable pins 0-3 of controller port 2 for data reception
// - Pin 5 signals that data has been received by our console (on the other console this will be bit 6 in LCP_Data)
// - Pin 6 signals that data has been sent by the other console (the other console should change bit 5 in its LCP_Data)
#define	LCP_BIT7_AND_BIT5 		0b10100000;

// Bits 5, 3, 2, 1, 0 are set to 1, others are 0. Used to set these bits in LCP_Ctrl, which means
// starting the data write cycle to port 2 (data transmission) by triggering an external interrupt on the slave console.
//
// I.e., on the master console, the LCP_masterCycle() method should be called from the main game code, which will set
// the bits in LCP_Ctrl to the state specified above, and on the slave console we will previously trigger an external
// interrupt that will halt the main game code execution and call the LCP_slave() method. This will occur when on the
// master console the 5th bit in LCP_Data is reset to 0 in the LCP_startSendCycle() method.
//
// LCP_Ctrl bit values:
// - Interrupt on SYN line signal disabled, bit 7 = 0
// - Data lines PC5, PC3, PC2, PC1, PC0 in output mode (transmission), bits 5, 3, 2, 1, 0 = 1
// - Data lines PC6, PC4 in input mode (reception). Corresponding bits = 0
//
// At the "hardware" level for my code this means:
// - Enable pins 0-3 of controller port 2 for data transmission
// - Pin 5 signals that data can be read by the other console (on the other console this will be bit 6 in LCP_Data)
// - Pin 6 signals that data has been read by the other console (the other console should change bit 5 in its LCP_Data)
#define LCP_BIT_5_3_2_1_0 		0b00101111;

// Mask 00001111. Mainly used to reset the high 4 bits in a byte to 0
#define LCP_LO_BITS 			0b00001111

// Mask 11110000. Mainly used to reset the low 4 bits in a byte to 0
#define LCP_HI_BITS  			0b11110000

// Timeout - used for waiting for data from the other console (waiting for reset or set of bit 6 in LCP_Data)
#define LCP_TIME_OUT 			0x40

// Maximum packet size for data transmission via Link Cable
#define LCP_PACKET_SIZE			2048


/**
 * Library function declarations
 */


/**
 * Opening port 2 of the console for data exchange via Link Cable
 * upon external interrupt - External Interrupt (EX-INT)
 * interrupt mask level 2 for Motorola 68000 processor
 * setting up a callback function called when EX-INT interrupt occurs
 */
void LCP_open();


/**
 * Initialization of console port 2 for data exchange via Link Cable
 * for direct data writing to the port and subsequent data reading from the port.
 * EX-INT interrupts are disabled for this console.
 * Reset of global variables.
 */
void LCP_init();


/**
 * Closing the console port for data exchange via Link Cable
 * disabling the external interrupt handler - External Interrupt (EX-INT)
 * interrupt mask level 2 for Motorola 68000 processor
 */
void LCP_close();


/**
 * Function for copying the transferObject as a byte array
 * into the LCP_sendPacket for transmission via Link Cable.
 *
 * Packet structure:
 * - First 2 bytes contain the type of transmitted object (cannot be 0)
 * - Followed by object data (2 bytes each) according to objectTypeSizes table
 *
 * Multiple objects can be transmitted at once, as long as they fit in the buffer.
 * It is assumed that the function can be called multiple times in sequence until
 * the errors described below occur.
 *
 * Error codes (error code = LCP_error & 0x000F):
 * 0xC - transmitted object size is negative or zero
 * 0xD - transmitted object size exceeds remaining packet space
 * 0xF - packet data start index is greater than data end index
 *
 * Parameters:
 * transferObject - object to send as byte array via Link Cable
 * objectType - type of object to send (number corresponding to index in object sizes array)
 * objectTypeSizes - array of object sizes (defined in YOUR main game code)
 */
void LCP_objectToPacketForSend(u8 *transferObject, u16 objectType , u16 *objectTypeSizes);


/**
 * Function extracts the next object as a byte array transferObject from the received LCP_ReceivePacket
 * via Link Cable and returns the type of this object or 0 if the object could not be extracted from LCP_ReceivePacket.
 *
 * Since multiple objects can be transmitted at once, it is assumed that this function can be called
 * multiple times in sequence until 0 is returned, meaning - all objects have been extracted from the transmitted packet.
 *
 * Packet structure:
 * - First 2 bytes of LCP_ReceivePacket contain the type of transmitted object (cannot be 0)
 * - Followed by object data (2 bytes each) according to objectSizes table
 * - Multiple objects can be transmitted at once, as long as they fit in the LCP_ReceivePacket array
 *
 * Error codes (error code = LCP_error & 0xF000):
 * 0x1000 - no object in received packet, all objects already extracted or nothing was transmitted
 * 0x2000 - size of object to extract from read packet buffer is negative or zero
 * 0x3000 - size of object to extract exceeds remaining data in received packet
 * 0x4000 - size of object to extract exceeds remaining size of LCP_ReceivePacket array
 *
 * Parameters:
 * transferObject - next received object as byte array via Link Cable
 * objectSizes - array of object sizes (defined in YOUR main game code)
 *
 * Returns:
 * type of received object or 0 if object could not be extracted from received packet
 */
u16 LCP_getNextObjectFromReceivePacket(u8 *transferObject, u16 *objectSizes);


/**
 * Data transmission and reception cycle via Link Cable for SEGA master console
 * At the very beginning, disable all interrupt processing and reset LCP_timeOut and LCP_error to 0.
 * Both in case of error and successful function completion (i.e. always), call LCP_masterCycleEnd() at the end
 * to set console port 2 to correct state and enable all interrupt processing.
 *
 * Error codes (error code = LCP_error & 0x00F0):
 * 0x10 - error calling LCP_startSendCycle() - start of data transmission from master console
 * 0x20 - error calling LCP_send(size) - transmission of 2 bytes (packet size) from master console
 * 0x30 - error calling LCP_send(data) - transmission of 2 bytes (packet data) from master console
 * 0x40 - error calling LCP_send(checksum) - transmission of 2 bytes (checksum) from master console
 * 0x50 - error calling LCP_endSendCycle() - end of main packet transmission cycle from master to slave console
 * 0x60 - error calling LCP_startReceiveCycle() - start of data reception cycle from slave to master console
 * 0x70 - error calling checksum = LCP_Receive() - reception of 2 bytes (packet checksum) from slave console
 * 0x80 - checksum calculated by master console != checksum received from slave console
 * 0x90 - error calling size = LCP_Receive() - reception of 2 bytes (packet size) from slave console
 * 0xA0 - error calling data = LCP_Receive() - reception of 2 bytes (packet data) from slave console
 * 0xB0 - error calling checksum = LCP_Receive() - main reception cycle of 2 bytes (checksum) from slave console
 * 0xC0 - checksum calculated by master console != checksum received from slave console
 * 0xD0 - error calling LCP_endReceiveCycle() - end of data reception cycle from slave console
 * 0xE0 - error calling LCP_startSendCycle() - start of checksum transmission from master to slave console
 * 0xF0 - error calling LCP_send(checksum) - transmission of checksum from master to slave console
 */
void LCP_masterCycle();


/**
 * Error in data transmission via SEGA Link Cable
 *
 * If 0x0000, no errors occurred!                                        Value ranges     in binary format
 * Lower 4 bits of low byte  - data reception errors via Link Cable    (0x0001 - 0x000F)  000000000000xxxx
 * Upper 4 bits of low byte  - data transmission errors via Link Cable (0x0010 - 0x00F0)  00000000xxxx0000
 * Lower 4 bits of high byte - packet reception or transmission errors (0x0100 - 0x0F00)  0000xxxx00000000
 * Upper 4 bits of high byte - object extraction errors from packet    (0x1000 - 0xF000)  xxxx000000000000
 *
 * return LCP_error
 */
u16 LCP_getError();

#endif // MODULE_LINK_CABLE

#endif // _LINK_CABLE_H
