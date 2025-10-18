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

#include "config.h"

#if (MODULE_LINK_CABLE != 0)

#include "ext/link_cable.h"

#include "types.h"
#include "sys.h"
#include "vdp.h"
#include "z80_ctrl.h"
#include "memory.h"


/**
 * Global variables
 */

// Port open or not (port CTRL 2 in SEGA documentation - second controller port)
//
// If 1 - open, external interrupt EX-INT (External Interrupt in SEGA documentation) should be triggered,
// initiated by another console which will be master relative to ours.
// I.e. we first receive data from the master console via port,
// then send data to it in one data exchange cycle.
//
// The interrupt handler can be called at any moment on our slave console.
// The executing code will be suspended at the moment of interrupt, and after the interrupt handler execution
// registers will be restored and main game code execution will continue.
//
// Data reception and transmission are asynchronous relative to preparation of what to send and processing of what
// was received by our slave console.
//
// If 0 - port closed, your console doesn't have EX-INT (External Interrupt) handler but can trigger
// this interrupt on another console, i.e. your console is master. We first send data to slave
// console, then receive data from it in one data exchange cycle.
//
// Data transmission and reception on our master console are synchronous, from main game code, relative to
// preparation of what to send and processing of what was received from another slave console
static u16 LCP_portOpen = 0;


// Start index of data in LCP_sendPacket array that needs to be transmitted via Link Cable
// Can transmit multiple objects, here will be the index of the first byte of the first
// untransmitted object in LCP_sendPacket packet
static u16 LCP_sendHead = 0;


// End index of data in LCP_sendPacket array that needs to be transmitted via Link Cable
// Can transmit multiple objects, here will be the last byte of the last
// untransmitted object in LCP_sendPacket packet
static u16 LCP_sendTail = 0;


// Data transmission packet - array where LCP_objectToPacketForSend function stores objects for transmission via Link Cable.
// This is done from the main loop of your game when something needs to be transmitted to another console.
// First 16 bits are the object type, followed by object data, and there can be multiple such objects in the packet.
//
// 0                       LCP_sendHead                                 LCP_sendTail                    LCP_PACKET_SIZE
// |   16 bits              |                                            |                                    |
// [object type 1][object1][object type 2][object2][object type 3][object3][][][][][][][][][][][][][][]][][]][]
static u16 LCP_sendPacket[LCP_PACKET_SIZE];


// Start index of data in LCP_ReceivePacket array that was received via Link Cable
// Can receive multiple objects at once, here will be the index of the first byte of the first
// received object in LCP_ReceivePacket array
static u16 LCP_ReceiveHead  = 0;


// End index of data in LCP_ReceivePacket array that was received via Link Cable
// Can receive multiple packets at once, here will be the last byte of the last
// received object in LCP_ReceivePacket array
static u16 LCP_ReceiveTail  = 0;


// Received data packet - array from which LCP_getNextObjectFromReceivePacket function in your game's main code copies
// the next received object into transferObject (pointer to byte array in your game code) and returns the type of this object.
// This is done in your game's main code, while data gets into the packet during Master cycle work - LCP_masterCycle() for master console
// or Slave cycle work - LCP_slaveCycle() for slave console. These functions handle data transmission via Link Cable.
// LCP_masterCycle() - always called synchronously from your game's main code! And only on master console.
// LCP_slaveCycle() - called asynchronously upon external interrupt - External Interrupt (EX-INT) only on slave console.
//
// 0                       LCP_ReceiveHead                               LCP_ReceiveTail                LCP_PACKET_SIZE
// |   16 bits              |                                            |                                    |
// [object type 1][object1][object type 2][object2][object type 3][object3][][][][][][][][][][][][][][]][][]][]
static u16 LCP_ReceivePacket[LCP_PACKET_SIZE];


// If non-zero, means there was a connection break during reception or transmission of lower 4 bits of LCP_Data
// and a data wait timeout occurred. What exactly happened can be understood from LCP_error.
static u8 LCP_timeOut = 0;


// Error in data transmission via SEGA Link Cable
//
// If 0x0000, no errors occurred!                                        Value ranges     in binary format
// Lower 4 bits of low byte  - data reception errors via Link Cable    (0x0001 - 0x000F)  000000000000xxxx
// Upper 4 bits of low byte  - data transmission errors via Link Cable (0x0010 - 0x00F0)  00000000xxxx0000
// Lower 4 bits of high byte - packet reception or transmission errors (0x0100 - 0x0F00)  0000xxxx00000000
// Upper 4 bits of high byte - object extraction errors from packet    (0x1000 - 0xF000)  xxxx000000000000
static u16 LCP_error = 0;

/**
 * Start of 2-byte (16-bit) data transfer cycle via Link Cable
 * Reset bit 5 of LCP_Data to 0 - data not ready for reading by other console
 * Set LCP_Ctrl = 00101111 to enable on controller port 2:
 * pins 0-3 for data transmission, and pin 5 for signal that data can be read
 * by other console. As soon as we set it to 1, we expect pin 6 to be 0
 * at function call time (if it's 1, we'll wait for 0 until timeout occurs and throw error 0xA if not received)
 *
 * Error codes (error code = LCP_error & 0x000F):
 * 0xA - data transmission preparation error (failed to get 0 on pin 6 of 2nd controller)
 *
 * If no errors occurred (LCP_error == 0 and LCP_timeOut == 0)
 */
static void LCP_startSendCycle();


/**
 * Main 2-byte (16-bit) data transfer cycle via Link Cable
 * Transmission is done through LCP_Data - shared memory area of both consoles
 * 4 bits at a time, i.e. in 4 iterations since 2 bytes = 16 bits
 * Fills the lower 4 data bits in LCP_Data that we're sending
 * Sets or resets bit 5 depending on iteration in LCP_Data
 * Waits for data reception confirmation from other console in bit 6 of LCP_Data
 * If confirmation is not received, saves error in LCP_error
 *
 * Error codes (error code = LCP_error & 0x000F):
 * 0x1 - failed to receive confirmation on 1st iteration
 * 0x2 - failed to receive confirmation on 2nd iteration
 * 0x3 - failed to receive confirmation on 3rd iteration
 * 0x4 - failed to receive confirmation on 4th iteration
 *
 * data - 2 bytes of data (16 bits) to transmit
 * Let's assume initial data = "aaaayyyybbbbxxxx" - split 2 bytes (16 bits) into 4-bit chunks:
 * first transmit aaaa, then yyyy, then bbbb, and in 4th iteration xxxx
 *
 * At function call time LCP_Data = "r00rrrrr" - where r is any value and doesn't matter currently
 * LCP_startSendCycle function must have completed successfully first, or another LCP_send must complete successfully
 *
 * If no errors occurred (LCP_error == 0 and LCP_timeOut == 0)
 */
static void LCP_send(u16 data);


/**
 * End of 2-byte (16-bit) data transfer cycle via Link Cable
 * Set LCP_Ctrl = 00100000 to enable on controller port 2:
 * pins 0-3 for data reception, and pin 5 for signal that our SEGA is ready to receive data.
 * Reset bit 5 in LCP_Data to 0, wait for confirmation that other console is ready to send data -
 * until bit 6 in LCP_Data becomes 1 or timeout occurs, and throw error 0xB if not received.
 *
 * Error codes (error code = LCP_error & 0x000F):
 * 0xB - data transfer cycle end error (failed to get 1 in bit 6 of LCP_Data)
 *
 * If no errors occurred (LCP_error == 0 and LCP_timeOut == 0)
 */
static void LCP_endSendCycle();


/**
 * Start of 2-byte (16-bit) data reception cycle via Link Cable
 * Reset bit 5 in LCP_Data to 0 (ready for reading)
 */
static void LCP_startReceiveCycle();


/**
 * Main 2-byte (16-bit) data reception cycle via Link Cable
 * Receives 4 bits at a time over 4 iterations through LCP_Data (lower 4 bits)
 * Sets or resets bit 5 in LCP_Data depending on iteration, thereby
 * confirming successful reading to the console that sent us data
 * Waits for confirmation in bit 6 of LCP_Data from other console that data can be read
 *
 * Error codes (error code = LCP_error & 0x000F):
 * 0x5 - failed to receive confirmation that data can be read on 1st iteration
 * 0x6 - failed to receive confirmation that data can be read on 2nd iteration
 * 0x7 - failed to receive confirmation that data can be read on 3rd iteration
 * 0x8 - failed to receive confirmation that data can be read on 4th iteration
 *
 * return - 2 bytes of data (16 bits) received from other SEGA "aaaayyyybbbbxxxx"
 *          if no errors occurred (LCP_error == 0 and LCP_timeOut == 0)
 */
static u16 LCP_Receive();


/**
 * End of 2-byte (16-bit) data reading cycle via Link Cable
 * Wait for confirmation that the other console understood the end of data reception from our console -
 * until bit 6 in LCP_Data becomes 0 or timeout occurs, and throw error 0x9 if not received.
 * Set bit 5 in LCP_Data to 1 - received confirmation from the other SEGA that it successfully transmitted everything.
 *
 * Error codes (error code = LCP_error & 0x000F):
 * 0x9 - failed to receive confirmation that data was received by the other SEGA
 *
 * If no errors occurred (LCP_error == 0 and LCP_timeOut == 0)
 */
static void LCP_endReceiveCycle();


/**
 * End of data transmission and reception cycle via Link Cable for SEGA master console
 * Set bit 5 in LCP_Ctrl to 1, others to 0, which means we no longer transmit data in bits 0-3
 * Set bit 5 in LCP_Data to 1 for our master console, on the other console (slave) bit 6 will also become 1
 * Enable processing of all interrupts
 */
static void LCP_masterCycleEnd();


/**
 * Data reception and then transmission cycle via Link Cable for SEGA slave console
 * This is an EX-INT external interrupt handler - External interrupt in SEGA documentation
 * It MUST NOT be called directly from game code!
 *
 * This function will be set as EX-INT external interrupt handler when LCP_open() is called from YOUR game code
 * and will be disabled as external interrupt handler when LCP_close() is called from YOUR game code
 *
 * Function call always occurs asynchronously, interrupting your game code execution at random, unpredictable time for main game code!
 *
 * At the very beginning, it disables all interrupt processing and resets LCP_timeOut and LCP_error to 0
 *
 * Error codes (error code = LCP_error & 0x0F00):
 * 0x100 - error calling LCP_slaveCycle() - second console port not open for data exchange
 * 0x200 - error calling LCP_startReceiveCycle() - start of data reception from master console
 * 0x300 - error calling size = LCP_Receive() - receiving size of transmitted data from master console
 * 0x400 - error calling data = LCP_Receive() - receiving next 2 bytes of packet data from master console
 * 0x500 - error calling checksum = LCP_Receive() - receiving checksum from master console
 * 0x600 - error calling LCP_endReceiveCycle() - end of data reception from master console
 * 0x700 - error calling LCP_startSendCycle() - start of data transmission from slave console
 * 0x800 - error calling LCP_send(checksum) - transmitting checksum from slave console
 * 0x900 - error calling LCP_send(size) - transmitting size of packet to be sent from slave console
 * 0xA00 - error calling LCP_send(data) - transmitting next 2 bytes of packet data from slave console
 * 0xB00 - error calling LCP_send(checksum) - transmitting checksum from slave console
 * 0xC00 - error calling LCP_endSendCycle() - end of data transmission from slave console
 * 0xD00 - error calling LCP_startReceiveCycle() - start of data reception from master console
 * 0xE00 - error calling data = LCP_Receive() - receiving checksum from master console
 *
 * Error codes (error code = LCP_error & 0x00F0):
 * 0x80 - checksum calculated by slave console <> checksum received from master console
 * 0xC0 - checksum calculated by slave console <> checksum received from master console
 */
static void LCP_slaveCycle();


/**
 * End of data reception and then transmission cycle via Link Cable for SEGA slave console
 * Switches console's 2nd port to data reception mode via Link Cable, on all pins except pin 5
 * Set bit 5 in LCP_Ctrl to 1, others to 0, which means we no longer transmit data in bits 0-3
 * Set bit 5 in LCP_Data to 1 for our slave console, on the other console (master) bit 6 will also become 1
 * Re-enables processing of all interrupts
 *
 * Error codes (error code = LCP_error & 0x0F00):
 * 0xF00 - error calling LCP_slaveCycleEnd() - end of data reception cycle from slave console
 */
static void LCP_slaveCycleEnd();


/**
 * Reset start and end indices of data in packet for transmission via Link Cable (LCP_sendPacket array)
 */
void LCP_clearSendHeadAndTail();


/**
 * Reset start and end indices of data in packet for reception via Link Cable (LCP_ReceivePacket array)
 */
void LCP_clearReceiveHeadAndTail();


/**
 * Library functions implementation
 */

/**
 * Opening console's 2nd port for data exchange via Link Cable
 * upon external interrupt occurrence - External interrupt (EX-INT)
 * interrupt mask level 2 for 68000 processor
 * setting callback function called upon EX-INT interrupt occurrence
 */
void LCP_open() {

    // capture Z80 coprocessor bus
    //
    // Write word $0100 to address $A11100
    // and wait in loop until bit D8 becomes equal to 0
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // Disable interrupts via SR - Status Register, since interrupt mask level 7 will be set for 68000 processor
    //
    // Status Register looks like this:
    // Bit 15 14 13 12 11 10  9  8   7  6  5  4  3  2  1  0
    //    ---------------------------------------------------
    //    | T| -| S| -| -| I2,I1,I0 | -| -| -| X| N| Z| V| C|
    //    ---------------------------------------------------
    // I2, I1, I0 - interrupt masks, define minimum level of serviced interrupt requests
    //
    // Total of 3 interrupt types: EX-INT, H-INT, V-INT
    //
    //          | I2 | I1 | I0 |
    //
    //  level 0 |  0    0    0 |  ---------> lowest priority
    //  level 1 |  0    0    1 |
    //  level 2 |  0    1    0 |  External interrupt (EX-INT)
    //  level 3 |  0    1    1 |
    //  level 4 |  1    0    0 |  Horizontal interrupt (H-INT)
    //  level 5 |  1    0    1 |
    //  level 6 |  1    1    0 |  Vertical interrupt (V-INT)
    //  level 7 |  1    1    1 |  ---------> highest priority
    //
    // When setting 7 = i.e. binary 111, I2 = 1, I1 = 1, I0 = 1
    // all interrupts are ignored, since their levels are all lower
    SYS_setInterruptMaskLevel(7);

    // Set function that will be called upon occurrence of
    // external interrupt EX-INT - External interrupt
    // i.e. game code execution will be interrupted upon EX-INT interrupt occurrence, register state will be saved
    // then the function passed to SYS_setExtIntCallback - LCP_slaveCycle() will be called, upon completion register state is restored and
    // game code will continue from where it was interrupted
    SYS_setExtIntCallback(LCP_slaveCycle);

    // Enable external interrupt (interrupt mask level 2 for 68000 processor)
    //
    // Register 11 looks like this:
    // Bit  7   6   5   4    3      2      1     0
    //    | - | - | - | - | IE2 | VSCR | HSCR | LSCR |
    //
    // Setting register 11 to 8 or in binary to 00001000
    // i.e. flags will take these values:
    // VSCR = 0 - vertical scroll mode = FULL SCROLL
    // HSCR = 0, LSCR = 0 - horizontal scroll mode = FULL SCROLL
    // IE2  = 1 - enables external interrupt (level 2 for 68000 External interrupt - EX-INT)
    VDP_setReg(11, 8);

    // ready for data exchange via console's 2nd port upon EX-INT interrupt occurrence
    // first reading data from port, then writing to port in external interrupt handler LCP_slaveCycle()
    // Interrupt on SYN line signal enabled, bit 7 = 1.
    // data line PC5 in output mode (transmission), bit 5 = 1.
    // data lines PC6, PC4, PC3, PC2, PC1, PC0 in input mode (reception). Corresponding bits = 0
    // enable pins 0-3 of 2nd controller port for data reception
    // pin 5 for signal that data is received by our console (on other console this will be bit 6 of LCP_Data)
    // pin 6 signal that data was sent by other console (other console should change bit 5 of its LCP_Data)
    LCP_Ctrl = LCP_BIT7_AND_BIT5;

    // Set bit 5 to 1 in LCP_Data (on other console bit 6 will become 1 in LCP_Data)
    LCP_Data |= LCP_BIT5;

    // flag that port is open and data exchange after EX-INT interrupt occurrence is allowed
    LCP_portOpen = 1;

    // Enable all interrupts via SR - Status Register
    //
    // set level 1 for 68000 processor, which means all interrupts will be processed
    // since their mask is higher than 1
    SYS_setInterruptMaskLevel(1);

    if (!busTaken) {
        // release Z80 bus
        // write word $0000 to address $A11100
        Z80_releaseBus();
    }
}


/**
 * Initialization of console's 2nd port for data exchange via Link Cable
 * for direct data writing to port and then reading data from port
 * EX-INT interrupts are disabled for this console
 * reset global variables
 */
void LCP_init() {
    // reset LCP_sendHead and LCP_sendTail to zero
    LCP_clearSendHeadAndTail();

    // reset LCP_ReceiveHead and LCP_ReceiveTail to zero
    LCP_clearReceiveHeadAndTail();

    // reset errors
    LCP_error = 0;

    // capture Z80 coprocessor bus
    bool busTaken = Z80_getAndRequestBus(TRUE);

    // flag that port is initialized and data exchange starts with writing to port without waiting for external interrupt occurrence
    // i.e. we trigger external interrupt on other console and after write cycle to port we start reading data from port
    LCP_portOpen = 0;

    // Disable all interrupts except V-INT via SR - Status Register, since interrupt mask level 5 will be set for 68000 processor
    //
    // Status Register looks like this:
    // Bit 15 14 13 12 11 10  9  8   7  6  5  4  3  2  1  0
    //    ---------------------------------------------------
    //    | T| -| S| -| -| I2,I1,I0 | -| -| -| X| N| Z| V| C|
    //    ---------------------------------------------------
    // I2, I1, I0 - interrupt masks, define minimum level of serviced interrupt requests
    //
    // Total of 3 interrupt types: EX-INT, H-INT, V-INT
    //
    //          | I2 | I1 | I0 |
    //
    //  level 0 |  0    0    0 |  ---------> lowest priority
    //  level 1 |  0    0    1 |
    //  level 2 |  0    1    0 |  External interrupt (EX-INT)
    //  level 3 |  0    1    1 |
    //  level 4 |  1    0    0 |  Horizontal interrupt (H-INT)
    //  level 5 |  1    0    1 |
    //  level 6 |  1    1    0 |  Vertical interrupt (V-INT)
    //  level 7 |  1    1    1 |  ---------> highest priority
    SYS_setInterruptMaskLevel(5);

    // Disable external interrupt (interrupt mask level 2 for 68000 processor)
    //
    // Setting register 11 to 0, i.e:
    // VSCR = 0 - vertical scroll mode = FULL SCROLL
    // HSCR = 0, LSCR = 0 - horizontal scroll mode = FULL SCROLL
    // IE2  = 0 - disables external interrupt (level 2 for 68000 processor External interrupt - EX-INT)
    VDP_setReg(11, 0);

    // ready for data exchange via console's 2nd port not upon interrupt occurrence, but
    // through direct writing to port and then reading from it. For this, you need to use LCP_masterCycle() in your game code
    //
    // Interrupt on SYN line signal disabled, bit 7 = 0.
    // data line PC5 in output mode (transmission), bit 5 = 1.
    // data lines PC6, PC4, PC3, PC2, PC1, PC0 in input mode (reception). Corresponding bits = 0
    // enable pins 0-3 of 2nd controller port for data reception
    // pin 5 for signaling events to other console (on other console bit 6 of LCP_Data will equal what we set in our 5th bit)
    // pin 6 signal from other console about events (other console should change bit 5 of its LCP_Data so that our 6th bit changes)
    LCP_Ctrl = LCP_BIT5;

    // Set bit 5 to 1 in LCP_Data (on other console bit 6 will become 1 in LCP_Data)
    LCP_Data |= LCP_BIT5;

    if (!busTaken) {
        // release Z80 bus
        // write word $0000 to address $A11100
        Z80_releaseBus();
    }
}


/**
 * Closing console port for data exchange via Link Cable
 * disable external interrupt handler - External interrupt (EX-INT)
 * interrupt mask level 2 for 68000 processor
 */
void LCP_close() {
    // disable EX-INT external interrupt handler - External interrupt in SEGA documentation
    SYS_setExtIntCallback(NULL);
    // reset LCP state
    LCP_init();
}


/**
 * Reset start and end indices of data in packet for transmission via Link Cable (LCP_sendPacket array)
 */
void LCP_clearSendHeadAndTail() {
    LCP_sendHead = 0;
    LCP_sendTail = 0;
}


/**
 * Reset start and end indices of data in packet for reception via Link Cable (LCP_ReceivePacket array)
 */
void LCP_clearReceiveHeadAndTail() {
    LCP_ReceiveHead  = 0;
    LCP_ReceiveTail  = 0;
}


/**
 * Start of 2-byte (16-bit) data transmission cycle via Link Cable
 * Reset bit 5 in LCP_Data to 0 - data not ready for reading by other console
 * Set LCP_Ctrl = 00101111 to enable on controller's 2nd port:
 * pins 0-3 for data transmission, and pin 5 for signal that data can be read
 * by other console. As soon as we set it to 1, we expect pin 6 to be 0
 * at function call time (if it's 1, we'll wait for 0 until timeout occurs and throw error 0xA if not received)
 *
 * Error codes (error code = LCP_error & 0x000F):
 * 0xA - data transmission preparation error (failed to get 0 on pin 6 of 2nd controller)
 *
 * If no errors occurred (LCP_error == 0 and LCP_timeOut == 0)
 */
static void LCP_startSendCycle() {

    // Reset bit 5 - data not ready for reading by other console yet
    LCP_Data &= ~(1 << 5);
    // LCP_Data = "r10rrrrr" - where r is any value and doesn't matter currently

    // set timeout for waiting for 0 in bit 6 of LCP_Data
    u16 timeout = LCP_TIME_OUT;

    do {
        timeout--;
        if (timeout == 0) {
            // failed to get 0 on pin 6 of 2nd controller
            LCP_timeOut++;
            // data transmission preparation error
            LCP_error = 0xA;
            return;
        }
    } while (LCP_Data & LCP_BIT6); // wait until bit 6 becomes 0 in LCP_Data

    // LCP_Data = "r00rrrrr" - where r is any value and doesn't matter currently

    // start data write cycle to 2nd port, write '00101111' to LCP_Ctrl
    // interrupt on SYN line signal disabled, bit 7 = 0.
    // data lines PC5, PC3, PC2, PC1, PC0 in output mode (transmission), bits 5, 3, 2, 1, 0 = 1.
    // data lines PC6, PC4 in input mode (reception). Corresponding bits = 0
    // enable pins 0-3 of 2nd controller port for data transmission
    // pin 5 for signal that data can be read by other console (on other console this will be bit 6 of LCP_Data)
    // pin 6 signal that data was read by other console (other console should change bit 5 of its LCP_Data)
    LCP_Ctrl = LCP_BIT_5_3_2_1_0;
}


/**
 * Main 2-byte (16-bit) data transmission cycle via Link Cable
 * Transmission is done through LCP_Data - shared memory area of both consoles
 * 4 bits at a time, i.e. in 4 iterations since 2 bytes = 16 bits
 * Fills the lower 4 data bits in LCP_Data that we're sending
 * Sets or resets bit 5 depending on iteration in LCP_Data
 * Waits for data reception confirmation from other console in bit 6 of LCP_Data
 * If confirmation is not received, saves error in LCP_error
 *
 * Error codes (error code = LCP_error & 0x000F):
 * 0x1 - failed to receive confirmation on 1st iteration
 * 0x2 - failed to receive confirmation on 2nd iteration
 * 0x3 - failed to receive confirmation on 3rd iteration
 * 0x4 - failed to receive confirmation on 4th iteration
 *
 * data - 2 bytes of data (16 bits) to transmit
 * Let's assume initial data = "aaaayyyybbbbxxxx" - split 2 bytes (16 bits) into 4-bit chunks:
 * first transmit aaaa, then yyyy, then bbbb, and in 4th iteration xxxx
 *
 * At function call time LCP_Data = "r00rrrrr" - where r is any value and doesn't matter currently
 * LCP_startSendCycle function must have completed successfully first, or another LCP_send must complete successfully
 *
 * If no errors occurred (LCP_error == 0 and LCP_timeOut == 0)
 */
static void LCP_send(u16 data) {
    // timeout
    u16 timeout;

    // temporary variable for manipulations with LCP_Data content
    u8 tmp = 0;

    // i - data transmission iteration (total of four iterations)
    u8 i = 1;
    do {
        // cyclic shift left by 4 (now lower 4 bits contain data we want to transmit)
        // data = "aaaayyyybbbbxxxx" was for i = 1, for i = 3 was data = "bbbbxxxxaaaayyyy"
        data = (data << 4) | (data >> 12);
        // data = "yyyybbbbxxxxaaaa" became for i = 1, for i = 3 became data = "xxxxaaaayyyybbbb"
        // we'll transmit aaaa or bbbb depending on iteration 1 or 3
        tmp = LCP_Data;
        // write 4 bits we want to transmit into lower 4 bits of LCP_Data
        // keep upper 4 bits from LCP_Data in place, i.e. (LCP_Data & 11110000)
        // and lower 4 bits of LCP_Data will be lower 4 bits from data, i.e. (data & 00001111)
        // LCP_Data = (LCP_Data & 11110000) | (data & 00001111)
        tmp = (tmp & LCP_HI_BITS) | (data & LCP_LO_BITS);
        LCP_Data = tmp;
        // after this operation:
        // LCP_Data = "r00raaaa" for i = 1, LCP_Data = "r00rbbbb" for i = 3

        // first and third iteration of data transmission to other SEGA
        // set bit 5 in LCP_Data to 1 (signal that data can be read by other SEGA)
        LCP_Data |= (1 << 5);
        // LCP_Data = "r01raaaa" for i = 1, LCP_Data = "r01rbbbb" for i = 3
        // On other SEGA, bit 6 in LCP_Data will change to 1, because pin 5 of our 2nd controller port
        // is connected to pin 6 of other SEGA's 2nd controller

        // set timeout
        timeout = LCP_TIME_OUT;

        // Wait for data to be read by other SEGA
        do {
            // decrease timeout value by 1
            timeout--;
            if (timeout == 0) {
                // failed to receive signal from other SEGA that data was read
                // Increase LCP_timeOut by 1
                LCP_timeOut++;

                // write i to LCP_error, which equals 1 or 3 (iteration)
                LCP_error = i;
                // LCP_error == 0x1 - failed to receive confirmation on 1st iteration
                // LCP_error == 0x3 - failed to receive confirmation on 3rd iteration

                // Exit function with error, LCP_error != 0
                return;
            }
          // wait until bit 6 becomes 1 in LCP_Data (signal that data was read by other SEGA)
          // Other SEGA should set bit 5 in its LCP_Data to 1, because its pin 5 of 2nd controller port
          // is connected to pin 6 of our SEGA's 2nd controller, and when it sets bit 5 to 1, our bit 6 will also become 1
        } while (!(LCP_Data & LCP_BIT6));
        // data transmission iteration completed successfully,
        // transmitted via Link Cable "aaaa" for 1st iteration or "bbbb" for 3rd iteration
        // LCP_Data = "r11raaaa" for i = 1, LCP_Data = "r11rbbbb" for i = 3

        // increase iteration number
        i++;
        // now i = 2 or i = 4

        // cyclic shift left by 4 (now lower 4 bits contain next data we want to transmit)
        // data = "yyyybbbbxxxxaaaa" was for i = 2, for i = 4 was data = "xxxxaaaayyyybbbb"
        data = (data << 4) | (data >> 12);
        // data = "bbbbxxxxaaaayyyy" became for i = 2, for i = 4 became data = "aaaayyyybbbbxxxx"
        tmp = LCP_Data;
        // write 4 bits we want to transmit into lower 4 bits of LCP_Data
        // Keep upper 4 bits from LCP_Data in place, i.e. (LCP_Data & 11110000)
        // and lower 4 bits of LCP_Data will be lower 4 bits from data, i.e. (data & 00001111)
        // LCP_Data = (LCP_Data & 11110000) | (data & 00001111)
        tmp = (tmp & LCP_HI_BITS) | (data & LCP_LO_BITS);
        LCP_Data = tmp;
        // after this operation:
        // LCP_Data = "r11ryyyy" for i = 2, LCP_Data = "r00rxxxx" for i = 4

        // second and fourth iteration of data transmission to other SEGA
        // set bit 5 in LCP_Data to 0 (signal that data can be read by other SEGA)
        LCP_Data &= ~(1 << 5);
        // LCP_Data = "r10ryyyy" for i = 2, LCP_Data = "r10rxxxx" for i = 4

        // set timeout again
        timeout = LCP_TIME_OUT;

        // Wait for data to be read by other SEGA
        do {
            // decrease timeout value by 1
            timeout--;
            if (timeout == 0) {
                // failed to receive signal from other SEGA that data was read
                // Increase LCP_timeOut by 1
                LCP_timeOut++;

                // write i to LCP_error, which equals 2 or 4 (iteration)
                LCP_error = i;
                // LCP_error == 0x2 - failed to receive confirmation on 2nd iteration
                // LCP_error == 0x4 - failed to receive confirmation on 4th iteration

                // Exit function with error, LCP_error != 0
                return;
            }
        // wait until bit 6 becomes 0 in LCP_Data (signal that data was read by other SEGA)
        } while (LCP_Data & LCP_BIT6);
        // data transmission iteration completed successfully,
        // transmitted via Link Cable "yyyy" for 2nd iteration or "xxxx" for 4th iteration
        // LCP_Data = "r00ryyyy" for i = 2, LCP_Data = "r00rxxxx" for i = 4

        // increase iteration number
        i++;
        // now i = 3 for 2nd iteration or i = 5 for 4th iteration

      // continue until i becomes 5, meaning everything transmitted, exit (2 cycles total - 2 transmission iterations each)
    } while (i < 5);
}


/**
 * End of 2-byte (16-bit) data transmission cycle via Link Cable
 * Set LCP_Ctrl = 00100000 to enable on controller's 2nd port:
 * pins 0-3 for data reception, and pin 5 for signal that our SEGA is ready to receive data.
 * Reset bit 5 in LCP_Data to 0, wait for confirmation that other console is ready to send data -
 * until bit 6 in LCP_Data becomes 1 or timeout occurs, and throw error 0xB if not received.
 *
 * Error codes (error code = LCP_error & 0x000F):
 * 0xB - data transmission cycle end error (failed to get 1 in bit 6 of LCP_Data)
 *
 * If no errors occurred (LCP_error == 0 and LCP_timeOut == 0)
 */
static void LCP_endSendCycle() {
    // set timeout
    u16 timeout = LCP_TIME_OUT;

    // switch port to data reception mode
    // Interrupt on SYN line signal disabled, bit 7 = 0.
    // data line PC5 in output mode (transmission), bit 5 = 1.
    // data lines PC6, PC4, PC3, PC2, PC1, PC0 in input mode (reception). Corresponding bits = 0
    // enable pins 0-3 of 2nd controller port for data reception
    // pin 5 for signaling events to other console (on other console bit 6 of LCP_Data will equal what we set in our 5th bit)
    // pin 6 signal from other console about events (other console should change bit 5 of its LCP_Data so that our 6th bit changes)
    LCP_Ctrl = LCP_BIT5;

    // Reset bit 5 to 0 in LCP_Data
    LCP_Data &= ~(1 << 5);
    // LCP_Data = "r00rrrrr" - where r is any value and doesn't matter currently

    // Wait for bit 6 in LCP_Data to become 1, other SEGA is ready to send data
    // and our console is ready to receive
    do {
        // decrease timeout value by 1
        timeout--;
        if (timeout == 0) {
            // failed to get 1 in bit 6 of LCP_Data, meaning second SEGA is not ready to send data
            LCP_timeOut++;
            // 0xB - data transmission cycle end error
            LCP_error = 0xB;
            // Exit function with error, LCP_error != 0
            return;
        }

      // wait until bit 6 becomes 1 in LCP_Data (signal that other SEGA is ready to send data to our SEGA)
    } while (!(LCP_Data & LCP_BIT6));

    // LCP_Data = "r10rrrrr" - where r is any value and doesn't matter currently

    // It is assumed that after executing this function, if we want to read data via Link Cable
    // we should call LCP_startReceiveCycle()
}


/**
 * Start of 2-byte (16-bit) data reception cycle via Link Cable
 * Reset bit 5 in LCP_Data to 0 (ready for reading)
 */
static void LCP_startReceiveCycle() {
    // Reset bit 5 in LCP_Data (ready for reading)
    LCP_Data &= ~(1 << 5);
    // LCP_Data = "r00rrrrr" - where r is any value and doesn't matter currently
}


/**
 * Main 2-byte (16-bit) data reception cycle via Link Cable
 * Receives 4 bits at a time over 4 iterations through LCP_Data (lower 4 bits)
 * Sets or resets bit 5 in LCP_Data depending on iteration, thereby
 * confirming successful reading to the console that sent us data
 * Waits for confirmation in bit 6 of LCP_Data from other console that data can be read
 *
 * Error codes (error code = LCP_error & 0x000F):
 * 0x5 - failed to receive confirmation that data can be read on 1st iteration
 * 0x6 - failed to receive confirmation that data can be read on 2nd iteration
 * 0x7 - failed to receive confirmation that data can be read on 3rd iteration
 * 0x8 - failed to receive confirmation that data can be read on 4th iteration
 *
 * return - 2 bytes of data (16 bits) received from other SEGA "aaaayyyybbbbxxxx"
 *          if no errors occurred (LCP_error == 0 and LCP_timeOut == 0)
 */
static u16 LCP_Receive() {
    // here will be data read from other SEGA, 16-bit variable (2 bytes)
    u16 data = 0;
    // data = "0000000000000000"

    // temporary variable for manipulations with LCP_Data content
    u8 tmp = 0;

    // timeout
    u16 timeout;

    // data transmission iteration (total of four iterations)
    // starting from 5 so reception errors differ from transmission errors
    // 5 - 1st reception iteration, 6 - 2nd, 7 - 3rd, 8 - 4th reception iteration
    u8 i = 5;

    do {
        // set timeout
        timeout  = LCP_TIME_OUT;

        // Wait for bit 6 in LCP_Data to become 1, other SEGA has sent data
        do {
            // decrease timeout value by 1
            timeout--;
            if (timeout == 0) {
                // failed to receive signal from other SEGA that data was sent to us
                // Increase LCP_timeOut
                LCP_timeOut++;

                // write i to LCP_error, which equals 5 or 7 (1st or 3rd iteration)
                LCP_error = i;
                // LCP_error == 0x5 - failed to receive confirmation that data can be read on 1st iteration
                // LCP_error == 0x7 - failed to receive confirmation that data can be read on 3rd iteration

                // If timeout reached 0, exit loop and return current data
                return data;
                // data = "0000000000000000" for 1st iteration (i = 5)
                // or data = "00000000aaaayyyy" for 3rd iteration (i = 7)
            }
        // loop continues until bit 6 becomes 1 - signal from other SEGA that data can be read by us
        // Other SEGA should set bit 5 in its LCP_Data to 1, because its pin 5 of 2nd controller port
        // is connected to pin 6 of our SEGA's 2nd controller, and when it sets bit 5 to 1, our bit 6 will also become 1
        } while (!(LCP_Data & LCP_BIT6));
        // data reception iteration completed successfully,
        // received via Link Cable "aaaa" for 1st iteration or "bbbb" for 3rd iteration
        // LCP_Data = "r10raaaa" for i = 5, LCP_Data = "r10rbbbb" for i = 7
        // temporary variable for manipulations with LCP_Data content
        tmp = LCP_Data;
        // now we need to save received 4 bits to data
        // for this we shift all bits in data left by 4 bits (data << 4)
        // for 1st iteration this doesn't matter since it's 0, but for 3rd (i = 7) we transform "00000000aaaayyyy" into "0000aaaayyyy0000"
        // now take only lower 4 bits from LCP_Data (LCP_Data & 00001111) and combine with result from first expression
        data = (data << 4) | (tmp & LCP_LO_BITS);
        // data = "000000000000aaaa" for 1st iteration (i == 5), "0000aaaayyyybbbb" for 3rd iteration (i == 7)

        // 1st or 2nd iteration of data reception from other SEGA completed, data saved to data
        // set bit 5 in LCP_Data to 1 (read data, notify other SEGA about this)
        LCP_Data |= (1 << 5);
        // LCP_Data = "r11raaaa" for 1st iteration (i == 5) or LCP_Data = "r11rbbbb" for 3rd iteration (i == 7)

        // increase iteration number
        i++;
        // now i = 6 for 2nd iteration or i = 8 for 4th

        // Set timeout again
        timeout = LCP_TIME_OUT;

        // Wait for bit 6 in LCP_Data to become 0, other SEGA has sent data
        do {
            // decrease timeout value by 1
            timeout--;
            if (timeout == 0) {
                // failed to receive signal from other SEGA that data was sent to us
                // Increase LCP_timeOut
                LCP_timeOut++;

                // write i to LCP_error, which equals 6 or 8 (2nd or 4th iteration)
                LCP_error = i;
                // LCP_error == 0x6 - failed to receive confirmation that data can be read on 2nd iteration
                // LCP_error == 0x8 - failed to receive confirmation that data can be read on 4th iteration

                // If timeout reached 0, exit loop and return current data
                return data;
                // data = "000000000000aaaa" for 1st iteration (i = 6)
                // or data = "0000aaaayyyybbbb" for 3rd iteration (i = 8)
            }
        // loop continues until bit 6 becomes 0 - signal from other SEGA that data can be read by us
        // Other SEGA should set bit 5 in its LCP_Data to 0, because its pin 5 of 2nd controller port
        // is connected to pin 6 of our SEGA's 2nd controller, and when it sets bit 5 to 0, our bit 6 will also become 0
        } while (LCP_Data & LCP_BIT6);
        // data reception iteration completed successfully,
        // received via Link Cable "yyyy" for 2nd iteration or "xxxx" for 4th iteration
        // LCP_Data = "r01ryyyy" for i = 6, LCP_Data = "r01rxxxx" for i = 8
        tmp = LCP_Data;
        // now we need to save received 4 bits to data
        // for this we shift all bits in data left by 4 bits (data << 4)
        // for 2nd iteration (i == 6) we transform "000000000000aaaa" into "00000000aaaa0000"
        // for 4th iteration (i = 8) we transform "0000aaaayyyybbbb" into "aaaayyyybbbb0000"
        // take only lower 4 bits from LCP_Data (LCP_Data & 00001111) and combine with result from first expression
        data = (data << 4) | (tmp & LCP_LO_BITS);
        // data = "00000000aaaayyyy" for 2nd iteration (i == 6), "aaaayyyybbbbxxxx" for 4th iteration (i == 8)

        // 2nd or 4th iteration of data reception from other SEGA completed, data saved to data
        // set bit 5 in LCP_Data to 0 (read data, notify other SEGA about this)
        LCP_Data &= ~(1 << 5);
        // LCP_Data = "r00ryyyy" for 2nd iteration (i == 6) or LCP_Data = "r00rxxxx" for 4th iteration (i == 8)

        // increase iteration number
        i++;
        // now i = 7 for 2nd iteration or i = 9 for 4th

    // continue until i becomes 9, meaning everything received, exit (2 cycles total - 2 reception iterations each)
    } while( i < 9);

    // return read 2 bytes (16 bits) via Link Cable
    return data;
}


/**
 * End of 2-byte (16-bit) data reading cycle via Link Cable
 * Wait for confirmation that the other console understood the end of data reception from our console -
 * until bit 6 in LCP_Data becomes 0 or timeout occurs, and throw error 0x9 if not received.
 * Set bit 5 in LCP_Data to 1 - received confirmation from the other SEGA that it successfully transmitted everything.
 *
 * Error codes (error code = LCP_error & 0x000F):
 * 0x9 - failed to receive confirmation that data was received by the other SEGA
 *
 * If no errors occurred (LCP_error == 0 and LCP_timeOut == 0)
 */
static void LCP_endReceiveCycle() {
    // set timeout
    u16 timeout = LCP_TIME_OUT;

    // wait for the other SEGA console to understand that we successfully read all 2 bytes of data from it
    do {
        // decrease timeout value by 1
        timeout--;
        if (timeout == 0) {
            LCP_timeOut++;
            // 0x9 - failed to receive confirmation that data was received by the other SEGA
            LCP_error = 0x9;
            // exit function with error in LCP_error and LCP_timeOut > 0
            return;
        }
      // loop continues until bit 6 in LCP_Data becomes 0 - signal from other SEGA that data was read
    } while (LCP_Data & LCP_BIT6);

    // after exiting the loop
    // LCP_Data = "r00rxxxx"

    // set bit 5 in LCP_Data to 1 - received confirmation from other SEGA that it successfully transmitted everything
    // LCP_Data = "r10rxxxx"
    LCP_Data |= (1 << 5);
}


/**
 * Function for copying transferObject - as byte array
 * into LCP_sendPacket for packet transmission via Link Cable
 * first 2 bytes in packet contain type of transmitted object (cannot be 0)
 * then packet contains object data (2 bytes each), according to object sizes table objectTypeSizes
 * multiple objects can be transmitted at once, main thing is they fit in buffer
 * i.e. function can be called multiple times in row, until we get
 * errors described below
 *
 * Error codes (error code = LCP_error & 0x000F):
 * 0xC - transmitted object size is negative or equal to 0
 * 0xD - transmitted object size exceeds remaining space in packet
 * 0xF - packet data start index is greater than data end index
 *
 * transferObject - transmitted object as byte array via Link Cable
 * objectType - type of transmitted object (number equal to index in object sizes array)
 * objectTypeSizes - array of object sizes (defined in main code of YOUR game)
 */
void LCP_objectToPacketForSend(u8 *transferObject, u16 objectType , u16 *objectTypeSizes) {
    // size of transmitted object from object sizes array
    u16 objectSize = objectTypeSizes[objectType];
    // end index of data in transmission packet LCP_sendPacket
    u16 tail = LCP_sendTail;
    // start index of data in transmission packet LCP_sendPacket
    u16 head = LCP_sendHead;

    // check that transmitted object size is greater than 0
    if (objectSize <= 0) {
        // 0xC - transmitted object size is negative or equal to 0
        LCP_error = 0xC;

        // exit function with error in LCP_error
        return;
    }

    // check that we won't exceed LCP_sendPacket array bounds when transmitting object
    if (objectSize > LCP_PACKET_SIZE - head) {
        // 0xD - transmitted object size exceeds remaining space in packet
        LCP_error = 0xD;

        // exit function with error in LCP_error
        return;
    }

    // check that data start index is not greater than data end index in packet
    if (head > tail) {
        // 0xF - packet data start index is greater than data end index
        LCP_error = 0xF;
        LCP_clearSendHeadAndTail();
        // exit function with error in LCP_error
        return;
    }

    // first 2 bytes (16 bits) of packet data will contain type of transmitted object
    LCP_sendPacket[tail] = objectType;

    // need to increase packet data end index by 1
    tail++;

    // copy transmitted object transferObject into data transmission packet - LCP_sendPacket, starting from index
    // tail with size objectSize
    memcpy(LCP_sendPacket + tail, transferObject, objectSize);

    // update value of data end index in LCP_sendPacket packet
    LCP_sendTail = tail + objectSize;
}


/**
 * Function extracts next object as byte array transferObject from received packet LCP_ReceivePacket
 * via Link Cable and returns type of this object or 0 if object couldn't be extracted from LCP_ReceivePacket
 * since we can transmit multiple objects at once, it's assumed possible to call this function multiple times
 * in row until we get 0, which means - all objects have been extracted from transmitted packet
 *
 * first 2 bytes of packet in LCP_ReceivePacket contain type of transmitted object (cannot be 0)
 * then packet contains object data (2 bytes each), according to object sizes table objectSizes
 * multiple objects can be transmitted at once, main thing is they fit in LCP_ReceivePacket array
 *
 * Error codes (error code = LCP_error & 0xF000):
 * 0x1000 - no object in received packet, all already extracted from packet or nothing was transmitted to us
 * 0x2000 - size of extracted object from read packets buffer - negative or equal to 0
 * 0x3000 - size of extracted object exceeds remaining data in received data packet
 * 0x4000 - size of extracted object exceeds remaining size of LCP_ReceivePacket array
 *
 * transferObject - next received object as byte array via Link Cable
 * objectSizes - array of object sizes (defined in main code of YOUR game)
 *
 * return - type of received object or 0 if couldn't extract object from received packet
 */
u16 LCP_getNextObjectFromReceivePacket(u8 *transferObject, u16 *objectSizes) {
    // current index in received packet from which we'll start reading data
    u16 head = LCP_ReceiveHead;
    // end index of data in received packet
    u16 tail = LCP_ReceiveTail;
    // object type that function will return, or 0 if no object was found in received packet
    u16 objectType;
    // object size from object sizes array according to object type
    u16 objectSize = 0;

    // Check for data presence in received packet (that data start doesn't match data end)
    if (head >= tail) {
        // 0x1000 - no object in received packet, all already extracted from packet or nothing was transmitted to us
        LCP_error |= 0x1000;
        // No data, return 0 (couldn't extract any object)
        return 0;
    }

    // read first byte - object type
    // it's assumed that objectSizes is declared somewhere in code of YOUR game !!!
    objectType = LCP_ReceivePacket[head];

    // since we read object type, we need to shift data start index by 1
    head++;

    // get object size by its type from object sizes table
    objectSize = objectSizes[objectType];

    // if object size is negative or equal to 0 - error!
    if (objectSize <= 0) {
        // 0x2000 - size of extracted object from read packets buffer - negative or equal to 0
        LCP_error |= 0x2000;
        // No data, return 0 (couldn't extract any object)
        return 0;
    }

    // If object size exceeds remaining data in read buffer - error!
    if (objectSize > tail - head) {
        // 0x3000 - size of extracted object exceeds remaining data in received data packet
        LCP_error |= 0x3000;
        // No data, return 0 (couldn't extract any object)
        return 0;
    }

    // If object size exceeds remaining size of array for data reception packet
    if (objectSize > LCP_PACKET_SIZE - head) {
        // 0x4000 - size of extracted object exceeds remaining size of LCP_ReceivePacket array
        LCP_error |= 0x4000;
        // No data, return 0 (couldn't extract any object)
        return 0;
    }

    // copy to byte array transferObject from received packet LCP_ReceivePacket starting from index head
    // objectSize bytes, i.e. entire object from LCP_ReceivePacket to transferObject
    memcpy(transferObject, LCP_ReceivePacket + head, objectSize);

    // since we extracted next object from packet, move start index of non-extracted objects in packet
    // to head + objectSize, from where on next call of LCP_getNextObjectFromReceivePacket we'll extract
    // next object
    LCP_ReceiveHead = head + objectSize;

    if (LCP_ReceiveHead == LCP_ReceiveTail) {
        // if everything was read, we can reset LCP_ReceiveHead and LCP_ReceiveTail to 0
        LCP_clearReceiveHeadAndTail();
    }

    // return type of object read into transferObject from received packet LCP_ReceivePacket
    return objectType;
}


/**
 * Transmission and then reception cycle via Link Cable for SEGA master console
 * at the very beginning disables all interrupt processing and resets LCP_timeOut and LCP_error to 0
 * both in case of error and in case of successful function completion, i.e. always call LCP_masterCycleEnd() at the end
 * to switch console's 2nd port to correct state and enable all interrupt processing.
 *
 * Error codes (error code = LCP_error & 0x00F0):
 * 0x10 - error calling LCP_startSendCycle() - start of data transmission from master console
 * 0x20 - error calling LCP_send(size) - transmission of 2 bytes (packet size) from master console
 * 0x30 - error calling LCP_send(data) - transmission of 2 bytes (packet data) from master console
 * 0x40 - error calling LCP_send(checksum) - transmission of 2 bytes (checksum) from master console
 * 0x50 - error calling LCP_endSendCycle() - end of main packet transmission cycle from master to slave console
 * 0x60 - error calling LCP_startReceiveCycle() - start of data reception cycle from slave console to master
 * 0x70 - error calling data = LCP_Receive() - reception of 2 bytes (packet checksum) from slave console
 * 0x80 - checksum calculated by master console <> checksum received from slave console
 * 0x90 - error calling size = LCP_Receive() - reception of 2 bytes (packet size) from slave console
 * 0xA0 - error calling data = LCP_Receive() - reception of 2 bytes (packet data) from slave console
 * 0xB0 - error calling data = LCP_Receive() - main cycle of receiving 2 bytes (checksum) from slave console
 * 0xC0 - checksum calculated by master console <> checksum received from slave console
 * 0xD0 - error calling LCP_endReceiveCycle() - end of data reception cycle from slave console
 * 0xE0 - error calling LCP_startSendCycle() - start of checksum transmission from master to slave console
 * 0xF0 - error calling LCP_send(checksum) - transmission of checksum from master to slave
 */
void LCP_masterCycle() {
    // size of transmitted or received data
    s16 size;
    // checksum for verifying transmitted data
    u16 checksum;
    // current index of transmitted or received data
    u16 head;
    // transmitted or received 2 bytes (16 bits) via Link Cable Protocol
    u16 data;

    // disable interrupt processing
    SYS_disableInts();

    // Clear timeout
    LCP_timeOut = 0;
    // Clear errors
    LCP_error = 0;

    // start of data transmission cycle via Link Cable from main console (master) to slave console
    // trigger external interrupt External interrupt (EX-INT) on slave console
    LCP_startSendCycle();

    if (LCP_timeOut) {
        // 0x10 - error calling LCP_startSendCycle() - start of data transmission from master console
        LCP_error |= 0x10;
        LCP_masterCycleEnd();
        // Exit function with error, LCP_error != 0
        return;
    }

    // calculate size of data to transmit based on end and start indices in transmission packet
    size = LCP_sendTail - LCP_sendHead;
    if (size < 0) {
        // if size less than 0, reset both indices and size of transmitted data
        LCP_clearSendHeadAndTail();
        size = 0;
    }

    // send packet size to slave console via Link Cable from master console
    LCP_send(size);

    if (LCP_timeOut) {
        // 0x20 - error calling LCP_send(size) - transmission of 2 bytes (packet size) from master console
        LCP_error |= 0x20;
        LCP_masterCycleEnd();
        // Exit function with error, LCP_error != 0
        return;
    }

    // reset checksum
    checksum = 0;

    // set index in transmission buffer to start of data in LCP_sendPacket
    head = LCP_sendHead;

    // transmit data if size greater than 0 (there is something to transmit)
    for (u16 i = 0; i < size; i++) {
        // extract next 2 bytes from packet data for transmission
        data = LCP_sendPacket[head];
        // increase index in transmission buffer by 1
        head++;
        // calculate checksum to verify successful data transmission
        checksum += data;
        // send next 2 bytes via Link Cable to slave console
        LCP_send(data);

        if (LCP_timeOut) {
            // 0x30 - error calling LCP_send(data) - transmission of 2 bytes (packet data) from master console
            LCP_error |= 0x30;
            LCP_masterCycleEnd();
            // Exit function with error, LCP_error != 0
            return;
        }
    }

    // send checksum to other console to verify previously transmitted data
    LCP_send(checksum);

    if (LCP_timeOut) {
        // 0x40 - error calling LCP_send(checksum) - transmission of 2 bytes (checksum) from master console
        LCP_error |= 0x40;
        LCP_masterCycleEnd();
        // Exit function with error, LCP_error != 0
        return;
    }
    // completion of data transmission cycle via Link Cable from master to slave console
    LCP_endSendCycle();

    if (LCP_timeOut) {
        // 0x50 - error calling LCP_endSendCycle() - end of main packet transmission cycle from master to slave console
        LCP_error |= 0x50;
        LCP_masterCycleEnd();
        // Exit function with error, LCP_error != 0
        return;
    }

    // start of data reception from slave SEGA in main cycle
    LCP_startReceiveCycle();

    if (LCP_timeOut) {
        // 0x60 - error calling LCP_startReceiveCycle() - start of data reception cycle from slave console to master
        LCP_error |= 0x60;
        LCP_masterCycleEnd();
        // Exit function with error, LCP_error != 0
        return;
    }

    // read first 2 bytes from slave console
    data = LCP_Receive();

    if (LCP_timeOut) {
        // 0x70 - error calling data = LCP_Receive() - reception of 2 bytes (packet checksum) from slave console
        LCP_error |= 0x70;
        LCP_masterCycleEnd();
        // Exit function with error, LCP_error != 0
        return;
    }

    // check that sum received from slave console equals checksum calculated in main cycle
    if (data == checksum) {
        // data successfully received by slave SEGA
        // reset start index of data to transmit and end index in buffer
        // since slave console received all data successfully
        LCP_clearSendHeadAndTail();
    } else {
        // 0x80 - checksum calculated by master console <> checksum received from slave console
        LCP_error |= 0x80;
        // don't interrupt function since no timeout occurred, try to receive data from slave console
        // next time data will be sent again!
    }

    // reset checksum
    checksum = 0;

    // receive first 2 bytes from slave console
    // size contains 2 bytes received from slave console since packet data size is stored there
    size = LCP_Receive();

    if (LCP_timeOut) {
        // 0x90 - error calling size = LCP_Receive() - reception of 2 bytes (packet size) from slave console
        LCP_error |= 0x90;
        LCP_masterCycleEnd();
        // Exit function with error, LCP_error != 0
        return;
    }

    // current index to start of data in LCP_ReceivePacket
    head = LCP_ReceiveTail;

    // receive data if size greater than 0 (there is something to receive)
    for (u16 i = 0; i < size; i++) {
        // receive next 2 bytes from slave console via Link Cable
        data = LCP_Receive();

        if (LCP_timeOut) {
            // 0xA0 - error calling data = LCP_Receive() - reception of 2 bytes (packet data) from slave console
            LCP_error |= 0xA0;
            LCP_masterCycleEnd();
            // Exit function with error, LCP_error != 0
            return;
        }

        // calculate checksum
        checksum += data;

        // save read 2 bytes into received objects data packet
        LCP_ReceivePacket[head] = data;

        // change current position index in received objects data packet
        head++;
    }

    // receive checksum from slave console
    data = LCP_Receive();

    if (LCP_timeOut) {
        // 0xB0 - error calling data = LCP_Receive() - main cycle of receiving 2 bytes (checksum) from slave console
        LCP_error |= 0xB0;
        LCP_masterCycleEnd();
        // Exit function with error, LCP_error != 0
        return;
    }

    // check that received checksum from slave console equals calculated one
    if (data == checksum) {
        // set data end index to current index in received data buffer
        LCP_ReceiveTail = head;
    } else {
        // 0xC0 - checksum calculated by master console <> checksum received from slave console
        LCP_error |= 0xC0;
        // therefore don't move end index for received data packet LCP_ReceivePacket
        // but since no timeouts occurred, continue function execution
    }

    // completion of data reception from slave console
    LCP_endReceiveCycle();

    if (LCP_timeOut) {
        // 0xD0 - error calling LCP_endReceiveCycle() - end of data reception cycle from slave console
        LCP_error |= 0xD0;
        LCP_masterCycleEnd();
        // Exit function with error, LCP_error != 0
        return;
    }

    // start of data transmission to slave console from master
    LCP_startSendCycle();

    if (LCP_timeOut) {
        // 0xE0 - error calling LCP_startSendCycle() - start of checksum transmission from master to slave console
        LCP_error |= 0xE0;
        LCP_masterCycleEnd();
        // Exit function with error, LCP_error != 0
        return;
    }

    // send checksum to slave console from master
    LCP_send(checksum);

    if (LCP_timeOut) {
        // 0xF0 - error calling LCP_send(checksum) - transmission of checksum from master to slave
        LCP_error |= 0xF0;
        LCP_masterCycleEnd();
        // Exit function with error, LCP_error != 0
        return;
    }

    // finish data transmission
    LCP_endSendCycle();

    // finish packet transmission/reception
    LCP_masterCycleEnd();
}


/**
 * End of data transmission and reception cycle via Link Cable for SEGA master console
 * switches console's 2nd port to data reception mode via Link Cable, on all pins except pin 5
 * Set bit 5 in LCP_Ctrl to 1, others to 0, which means we no longer transmit data in bits 0-3
 * Set bit 5 in LCP_Data to 1 for our master console, on the other console (slave) bit 6 will also become 1
 * re-enables processing of all interrupts
 */
static void LCP_masterCycleEnd() {
    // switch port to data reception mode
    // Interrupt on SYN line signal disabled, bit 7 = 0.
    // data line PC5 in output mode (transmission), bit 5 = 1.
    // data lines PC6, PC4, PC3, PC2, PC1, PC0 in input mode (reception). Corresponding bits = 0
    // enable pins 0-3 of 2nd controller port for data reception
    // pin 5 for signaling events to other console (on other console bit 6 of LCP_Data will equal what we set in our 5th bit)
    // pin 6 signal from other console about events (other console should change bit 5 of its LCP_Data so that our 6th bit changes)
    LCP_Ctrl = LCP_BIT5;

    // set bit 5 in LCP_Data to 1
    LCP_Data |= (1 << 5);

    // enable processing of all interrupts
    SYS_enableInts();
}


/**
 * Reception and then transmission cycle via Link Cable for SEGA slave console
 * This is an EX-INT external interrupt handler - External interrupt in SEGA documentation
 * It MUST NOT be called directly from game code!
 *
 * This function will be set as EX-INT external interrupt handler when LCP_open() is called from YOUR game code
 * and will be disabled as external interrupt handler when LCP_close() is called from YOUR game code
 *
 * Function call always occurs asynchronously, interrupting your game code execution at random, unpredictable time for main game code!
 *
 * At the very beginning, it disables all interrupt processing and resets LCP_timeOut and LCP_error to 0
 *
 * Error codes (error code = LCP_error & 0x0F00):
 * 0x100 - error calling LCP_slaveCycle() - second console port not open for data exchange
 * 0x200 - error calling LCP_startReceiveCycle() - start of data reception from master console
 * 0x300 - error calling size = LCP_Receive() - receiving size of transmitted data from master console
 * 0x400 - error calling data = LCP_Receive() - receiving next 2 bytes of packet data from master console
 * 0x500 - error calling data = LCP_Receive() - receiving checksum from master console
 * 0x600 - error calling LCP_endReceiveCycle() - end of data reception from master console
 * 0x700 - error calling LCP_startSendCycle() - start of data transmission from slave console
 * 0x800 - error calling LCP_send(checksum) - transmitting checksum from slave console
 * 0x900 - error calling LCP_send(size) - transmitting size of packet to be sent from slave console
 * 0xA00 - error calling LCP_send(data) - transmitting next 2 bytes of packet data from slave console
 * 0xB00 - error calling LCP_send(checksum) - transmitting checksum from slave console
 * 0xC00 - error calling LCP_endSendCycle() - end of data transmission from slave console
 * 0xD00 - error calling LCP_startReceiveCycle() - start of data reception from master console
 * 0xE00 - error calling data = LCP_Receive() - receiving checksum from master console
 *
 * Error codes (error code = LCP_error & 0x00F0):
 * 0x80 - checksum calculated by slave console <> checksum received from master console
 * 0xC0 - checksum calculated by slave console <> checksum received from master console
 */
static void LCP_slaveCycle() {
    // size of transmitted or received data
    s16 size;
    // checksum for verifying transmitted data
    u16 checksum = 0;
    // current index of transmitted or received data
    u16 head = LCP_ReceiveTail;
    // transmitted or received 2 bytes (16 bits) via Link Cable
    u16 data;

    LCP_error = 0;
    LCP_timeOut = 0;

    // switch port to data reading mode
    // Interrupt on SYN line signal disabled, bit 7 = 0.
    // data line PC5 in output mode (transmission), bit 5 = 1.
    // data lines PC6, PC4, PC3, PC2, PC1, PC0 in input mode (reception). Corresponding bits = 0
    // enable pins 0-3 of 2nd controller port for data reception
    // pin 5 for signaling events to other console (on other console bit 6 of LCP_Data will equal what we set in our 5th bit)
    // pin 6 signal from other console about events (other console should change bit 5 of its LCP_Data so that our 6th bit changes)
    LCP_Ctrl = LCP_BIT5;

    if (!LCP_portOpen) {
        // port not open
        // 0x100 - error calling LCP_slaveCycle() - second console port not open for data exchange
        LCP_error |= 0x100;
        return;
    }

    // disable all interrupt processing
    SYS_disableInts();

    // start data reception cycle for our slave console via Link Cable from master console
    LCP_startReceiveCycle();

    if (LCP_timeOut) {
        // 0x200 - error calling LCP_startReceiveCycle() - start of data reception from master console
        LCP_error |= 0x200;
        LCP_slaveCycleEnd();
        return;
    }

    // first 2 bytes of data - size of packet transmitted to our console (slave) via Link Cable from master console
    size = LCP_Receive();

    if (LCP_timeOut) {
        // 0x300 - error calling size = LCP_Receive() - receiving size of transmitted data from master console
        LCP_error |= 0x300;
        LCP_slaveCycleEnd();
        return;
    }

    // receive packet body from master console via Link Cable by our console (slave)
    for (u16 i = 0; i < size; i++) {
        // get 2 bytes of data into data by our slave console via Link Cable from master console
        data = LCP_Receive();

        if (LCP_timeOut) {
            // 0x400 - error calling data = LCP_Receive() - receiving next 2 bytes of packet data from master console
            LCP_error |= 0x400;
            LCP_slaveCycleEnd();
            return;
        }

        // calculate verification sum
        checksum += data;
        // save received 2 bytes into reception packet at index head
        LCP_ReceivePacket[head] = data;
        // increase head - start index of data in LCP_ReceivePacket by 1
        head++;
    }

    // after packet body, receive checksum of sent data in data from master console
    data = LCP_Receive();

    if (LCP_timeOut) {
        // 0x500 - error calling data = LCP_Receive() - receiving checksum from master console
        LCP_error |= 0x500;
        LCP_slaveCycleEnd();
        return;
    }

    // if verification sum received from master console in data equals our calculated one,
    // then save head index into global variable LCP_ReceiveTail - end index of received packet data
    // from master console
    if (data == checksum) {
        LCP_ReceiveTail = head;
    } else {
        // if LCP_error & 0x00F0 = 0x80 - checksum calculated by slave console <> checksum received from master console
        LCP_error |= 0x80;
        // don't interrupt function since no timeout occurred, try to send data from our slave console to master
        // and checksum calculated by our console, so master understands that data needs to be sent again
    }

    // end of data reception cycle for our slave console from master console via Link Cable
    LCP_endReceiveCycle();

    if (LCP_timeOut) {
        // 0x600 - error calling LCP_endReceiveCycle() - end of data reception from master console
        LCP_error |= 0x600;
        LCP_slaveCycleEnd();
        return;
    }

    // start data transmission cycle from our slave console via Link Cable to master console
    LCP_startSendCycle();

    if (LCP_timeOut) {
        // 0x700 - error calling LCP_startSendCycle() - start of data transmission from slave console
        LCP_error |= 0x700;
        LCP_slaveCycleEnd();
        return;
    }

    // transmit calculated checksum to master console from our slave console via Link Cable
    LCP_send(checksum);

    if (LCP_timeOut) {
        // 0x800 - error calling LCP_send(checksum) - transmitting checksum from slave console
        LCP_error |= 0x800;
        LCP_slaveCycleEnd();
        return;
    }

    // calculate size of packet to transmit
    size = LCP_sendTail - LCP_sendHead;
    if (size < 0) {
        size = 0;
        LCP_clearSendHeadAndTail();
    }

    // transmit packet body size from our slave console via Link Cable to master console
    LCP_send(size);

    if (LCP_timeOut) {
        // 0x900 - error calling LCP_send(size) - transmitting size of packet to be sent from slave console
        LCP_error |= 0x900;
        LCP_slaveCycleEnd();
        return;
    }

    // reset checksum value
    checksum = 0;

    // set head index to start of packet to be transmitted
    head = LCP_sendHead;

    // transmit packet body from our console (slave) via Link Cable to master console
    for (u16 i = 0; i < size; i++) {
        // write 2 bytes to data that we'll transmit from slave console packet
        data = LCP_sendPacket[head];

        // increase head index by 1 for start of data in LCP_sendPacket to be sent
        head++;

        // calculate verification sum
        checksum += data;

        // transmit 2 bytes of packet body from our slave console via Link Cable to master console
        LCP_send(data);

        if (LCP_timeOut) {
            // 0xA00 - error calling LCP_send(data) - transmitting next 2 bytes of packet data from slave console
            LCP_error |= 0xA00;
            LCP_slaveCycleEnd();
            return;
        }
    }

    // after packet body, transmit checksum of sent packet data
    // from our slave console via Link Cable to master console
    LCP_send(checksum);

    if (LCP_timeOut) {
        // 0xB00 - error calling LCP_send(checksum) - transmitting checksum from slave console
        LCP_error |= 0xB00;
        LCP_slaveCycleEnd();
        return;
    }

    // complete data transmission cycle from our slave console via Link Cable to master console
    LCP_endSendCycle();

    if (LCP_timeOut) {
        // 0xC00 - error calling LCP_endSendCycle() - end of data transmission from slave console
        LCP_error |= 0xC00;
        LCP_slaveCycleEnd();
        return;
    }

    // start data reception cycle for our slave console via Link Cable from master console
    LCP_startReceiveCycle();

    if (LCP_timeOut) {
        // 0xD00 - error calling LCP_startReceiveCycle() - start of data reception from master console
        LCP_error |= 0xD00;
        LCP_slaveCycleEnd();
        return;
    }

    // receive checksum of received data by master console via Link Cable from our slave console
    data = LCP_Receive();

    if (LCP_timeOut) {
        // 0xE00 - error calling data = LCP_Receive() - receiving checksum from master console
        LCP_error |= 0xE00;
        LCP_slaveCycleEnd();
        return;
    }

    if (data == checksum) {
        // if received checksum in data from master console equals transmitted and calculated by our console (slave),
        // then set packet start index and packet end index to 0 (everything successfully sent)
        LCP_clearSendHeadAndTail();
    } else {
        // 0xC0 - checksum calculated by slave console <> checksum received from master console
        LCP_error |= 0xC0;
        // therefore don't move start index for received data packet LCP_sendPacket
        // but since no timeouts occurred, continue function execution
    }

    // end of data reception cycle for our slave console from master console via Link Cable
    LCP_endReceiveCycle();

    // end of console interaction in this cycle from our slave side
    LCP_slaveCycleEnd();
}


/**
 * End of data reception and then transmission cycle via Link Cable for SEGA slave console
 * switches console's 2nd port to data reception mode via Link Cable, on all pins except pin 5
 * Set bit 5 in LCP_Ctrl to 1, others to 0, which means we no longer transmit data in bits 0-3
 * Set bit 5 in LCP_Data to 1 for our slave console, on the other console (master) bit 6 will also become 1
 * re-enables processing of all interrupts
 *
 * Error codes (error code = LCP_error & 0x0F00):
 * 0xF00 - error calling LCP_slaveCycleEnd() - end of data reception cycle from slave console
 */
static void LCP_slaveCycleEnd() {
    // switch port to data reading mode
    // Interrupt on SYN line signal disabled, bit 7 = 0.
    // data line PC5 in output mode (transmission), bit 5 = 1.
    // data lines PC6, PC4, PC3, PC2, PC1, PC0 in input mode (reception). Corresponding bits = 0
    // enable pins 0-3 of 2nd controller port for data reception
    // pin 5 for signaling events to other console (on other console bit 6 of LCP_Data will equal what we set in our 5th bit)
    // pin 6 signal from other console about events (other console should change bit 5 of its LCP_Data so that our 6th bit changes)
    LCP_Ctrl = LCP_BIT5;

    // set bit 5 in LCP_Data to 1
    LCP_Data |= (1 << 5);

    u16 timeout = LCP_TIME_OUT;
    do {
        timeout--; // Subtract 1 from timeout
        if (timeout == 0) {
            LCP_timeOut++;
            // 0xF00 - error calling LCP_slaveCycleEnd() - end of data reception cycle from slave console
            LCP_error |= 0xF00;
            // If timeout reached 0, exit loop
            break;
        }
        // wait until bit 6 becomes 1
    } while (!(LCP_Data & (1 << 6)));

    // ready for data exchange via console's 2nd port upon EX-INT interrupt occurrence
    // first reading data from port, then writing to port in external interrupt handler LCP_slaveCycle()
    // Interrupt on SYN line signal enabled, bit 7 = 1.
    // data line PC5 in output mode (transmission), bit 5 = 1.
    // data lines PC6, PC4, PC3, PC2, PC1, PC0 in input mode (reception). Corresponding bits = 0
    // enable pins 0-3 of 2nd controller port for data reception
    // pin 5 for signal that data is received by our console (on other console this will be bit 6 of LCP_Data)
    // pin 6 signal that data was sent by other console (other console should change bit 5 of its LCP_Data)
    LCP_Ctrl = LCP_BIT7_AND_BIT5;

    // enable processing of all interrupts
    SYS_enableInts();
}


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
u16 LCP_getError() {
	return LCP_error;
}

#endif // MODULE_LINK_CABLE
