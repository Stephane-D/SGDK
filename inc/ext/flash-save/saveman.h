/**
 * \file saveman.h
 * \brief Save manager for flash memory chips supported by the flash module.
 * \author Jesus Alonso (doragasu)
 * \date 10/2022
 *
 * This module allows saving data to non volatile flash chips. It supports as
 * many save slots as required, and arbitrary data lengths can be written to
 * each slot. This allows defining for example a 5 slot layout that uses slot
 * 0 for the game configuration, slot 1 to keep high scores and slots 2 to 4 to
 * save games. Each time data is written to a slot, it replaces whatever was
 * written to the slot previously.
 *
 * The module uses two sectors from the flash chip in order to implement flash
 * wear leveling and safe save: the save file is not lost even if there is a
 * power cut during save operation. Note that typical flash chips used in carts
 * such as Krikkzz Flash Kit Cart use 64 KiB sectors at the end of the flash, so
 * if you want to use this module, it will use the top 128 KiB of the ROM
 * address space by default. Keep this in mind because if code/data falls there,
 * it will be erased by this module routines, breaking your game! There is also
 * a way to use 8 KiB sectors from these carts, in order to waste 16 KiB instead
 * of 128 KiB, but it is a bit tricky, so unless you are very tight on ROM
 * space, better use the default layout. Using the default 128 KiB layout has two
 * additional benefits: wear is reduced and bigger save lengths are possible.
 *
 * Note when you define the slot layout, you must make sure all of them fit in
 * a single sector (plus a bit more room for internal headers). So e.g. if you
 * use 64 KiB sectors and have 5 slots with 2 KiB each, that will fit without a
 * problem, but if you use 8 KiB sectors, that config will not fit (5 x 2 KiB =
 * 10 KiB, bigger than the 8 KiB available in the sector). Make sure your
 * sector configuration is correct, because if it isn't, the module will work
 * initially, but will fail when more slots are saved and they do not fit in
 * one sector.
 *
 * Typical usage of the module is as follows:
 *
 * 1. Call sm_init() to initialize the module. Make sure it is always called
 *    with the same parameters.
 * 2. Call sm_load() to load data, sm_save() to save data and sm_delete() to
 *    delete saved data.
 * 3. In case you need to perform a factory reset, call sm_clear().
 *
 * The module has been carefully written, but some of its logic is a bit
 * complex, so make sure you test it thoroughly just in case there is a bug
 * lurking inside!
 *
 * \note Excepting sm_load(), all module functions stop Z80 and disable
 * interrupts until they return. Thus you shall make sure no music or sound
 * is playing when you call them.
 * \warning Save/load buffers must be word aligned or you will get a
 * SM_STAT_INVALID_PARAM_ERR error. Variables using types with a length equal
 * or bigger than 2 bytes will be aligned (same as structs with at least one
 * variable obeying this rule), but if variable has a type with length 1 (such
 * as a char array), to make sure it is aligned, you can use the attribute:
 * __attribute__((aligned(2))) in the declaration.
 * \warning If any save write operation returns a SM_STAT_HW_ERR, something
 * weird has occurred. Your best chance if this happens is to either restart
 * the console or at least deinit and init this module to let the init routine
 * try correcting the mess and avoid further data loss.
 */

#ifndef _SAVEMAN_H_
#define _SAVEMAN_H_

#include "types.h"

/**
 * \brief Status code for some of the function calls
 * \{/
 */
enum {
	SM_STAT_PARAM_ERR = -3,   ///< Invalid parameter in function call
	SM_STAT_HW_ERR = -2,      ///< Flash chip hardware related error
	SM_STAT_ERR = -1,         ///< Generic error
	SM_STAT_OK = 0,           ///< Success
	SM_STAT_WARN_NO_DATA = 1, ///< Success, chip has no data saved
};
/** \} */

/**
 * \brief Initialise save manager module. Must be called once before invoking
 * any other function in the module.
 *
 * \param[in] num_slots Number of save slots to support.
 * \param[in] max_length_restrict Maximum flash length the module can use. The
 *            module uses the last two sectors available. If you set this to 0,
 *            the module defaults to use the last two sectors in the 4 MiB
 *            range. But if you want to restrict it e.g. to 2 MiB to use a
 *            smaller chip, you can set it e.g. to 0x200000. This can also be
 *            used to alloc the smaller sectors in the flash chip, by pointing
 *            to the end of two of these sectors.
 *
 * \return Status code:
 * - SM_STAT_OK if initialisation was successful and flash has data to load.
 * - SM_STAT_WARN_NO_DATA if initialisation was successful but there is no
 *   save data to load.
 * - SM_STAT_PARAM_ERR if function was called with invalid parameters.
 * - SM_STAT_HW_ERR if the flash chip failed to perform any operation.
 * - SM_ERR if other unrecoverable error was found.
 *
 * \note This function internally calls flash_init(), you must not call it
 * by yourself.
 * \note This function halts Z80 and disables interrupts until it done.
 * \warning This module uses the last two sectors available in the specified
 * range. Typically this is from 0x3E0000 to 0x3FFFFF for the default value of
 * max_length_restrict set to 0. You must make sure there is no code in that
 * area, or it will be erased by this module!
 */
int16_t sm_init(uint8_t num_slots, uint32_t max_length_restrict);

/**
 * \brief Deinitialises save manager module. Usually you do not have to use
 * this function.
 */
void sm_deinit(void);

/**
 * \brief Load previously saved data for the specified slot.
 *
 * \param[in]  slot Slot from which we want to load the data.
 * \param[out] save_data Buffer in which loaded data will be copied.
 * \param[in]  len Length of the data to load.
 *
 * \return A positive number with the loaded data length, or a negative
 * error status code:
 * - SM_STAT_PARAM_ERR: Invalid parameter in invocation.
 * - SM_STAT_ERR: The requested slot has no data available.
 *
 * \note Readed bytes can be less than len if there is an error (e.g. 0 if the
 * slot has no data, or less than len if less data than requested was saved).
 */
int16_t sm_load(uint8_t slot, void *save_data, uint16_t len);

/**
 * \brief Save data to the specified slot.
 *
 * \param[in] slot Slot to which we want to save the data.
 * \param[in] save_data Data buffer to save in the slot.
 * \param[in] len Length of the data to save.
 *
 * \return Status code:
 * - SM_STAT_OK: Save operation successful.
 * - SM_STAT_PARAM_ERR: Invalid parameter in invocation.
 * - SM_STAT_HW_ERR: Save operation failed due to flash error.
 * - SM_STAT_ERR: Other unrecoverable error.
 *
 * \note Readed bytes can be less than len if there is an error.
 * \note This function halts Z80 and disables interrupts until it done.
 */
int16_t sm_save(uint8_t slot, const void *save_data, uint16_t len);

/**
 * \brief Delete data from specified slot
 *
 * \param[in] slot Slot to delete.
 *
 * \return Status code:
 * - SM_STAT_OK: Delete operation successful.
 * - SM_STAT_PARAM_ERR: Invalid parameter in invocation.
 * - SM_STAT_HW_ERR: Delete operation failed due to flash error.
 * - SM_STAT_ERR: Other unrecoverable error.
 *
 * \note This function halts Z80 and disables interrupts until it done.
 */
int16_t sm_delete(uint8_t slot);

/**
 * \brief Deletes all save data from all save slots, and reinitializes the
 * module to support the requested number of save slots.
 *
 * \param[in] num_slots Number of slots to support after data clear.
 *
 * \return Status code:
 * - SM_STAT_OK: Clear operation successful.
 * - SM_STAT_HW_ERR: Flash chip failed to erase.
 *
 * \note This function halts Z80 and disables interrupts until it done.
 */
int16_t sm_clear(uint8_t num_slots);

#endif
