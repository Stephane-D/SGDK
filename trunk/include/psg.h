/**
 * \file psg.h
 * \brief PSG support
 * \author Stephane Dallongeville
 * \date 08/2011
 *
 * This unit provides access to the PSG through the 68000 CPU
 */

#ifndef _PSG_H_
#define _PSG_H_

/**
 *  \def PSG_PORT
 *      PSG port address.
 */
#define PSG_PORT            0xC00011

/**
 *  \def PSG_ENVELOPE_MIN
 *      Minimum PSG envelope value.
 */
#define PSG_ENVELOPE_MIN    15
/**
 *  \def PSG_ENVELOPE_MAX
 *      Maximum PSG envelope value.
 */
#define PSG_ENVELOPE_MAX    0


/**
 * \brief
 *      Initialize PSG chip
 */
void PSG_init();

/**
 * \brief
 *      Write to PSG port.
 *
 * \param data
 *      value to write to the port.
 *
 * Write the specified value to PSG data port.
 *
 */
void PSG_write(u8 data);

/**
 * \brief
 *      Set envelope level.
 *
 * \param channel
 *      Channel we want to set envelope (0-3).
 * \param value
 *      Envelope level to set (PSG_ENVELOPE_MIN-PSG_ENVELOPE_MAX).
 *
 * Set envelope level for the specified PSG channel.
 */
void PSG_setEnvelope(u8 channel, u8 value);
/**
 * \brief
 *      Set tone.
 *
 * \param channel
 *      Channel we want to set tone (0-3).
 * \param value
 *      Tone value to set (0-1023).
 *
 * Set direct tone value for the specified PSG channel.
 */
void PSG_setTone(u8 channel, u16 value);
/**
 * \brief
 *      Set frequency.
 *
 * \param channel
 *      Channel we want to set frequency (0-3).
 * \param value
 *      Frequency value to set in Hz (0-4095).
 *
 * Set frequency for the specified PSG channel.<br>
 * This method actually converts the specified frequency value in PSG tone value.
 */
void PSG_setFrequency(u8 channel, u16 value);


#endif // _PSG_H_
