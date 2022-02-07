/************************************************************************//**
 * \file
 *
 * \brief JSON implementation, based on jsmn by Serge Zaitsev.
 *
 * \defgroup json json
 * \{
 *
 * \brief JSON implementation, based on jsmn by Serge Zaitsev.
 *
 * This module just adds some helper functions to jsmn to ease navigating
 * the JSON structure and extracting values.
 *
 * \author Jesus Alonso (doragasu)
 * \date 2019
 *
 * \todo Add more functions to extract data.
 ****************************************************************************/

#ifndef _JSON_H_
#define _JSON_H_

#include "types.h"

#if (MODULE_MEGAWIFI != 0)

/// Hides jsmn API definitions to avoid linking errors
#define JSMN_STATIC
#include "jsmn.h"

/// Macro to extract the item (as string) from a token. Requires the json_str
/// to have the null terminations added by json_null_terminate().
#define json_item(json_str, obj_tok, idx) (idx) < 0 ? NULL : \
	&((json_str)[(obj_tok)[idx].start])

/************************************************************************//**
 * \brief Add null-terminators to the input JSON string. Useful to further
 * extract token values as null-terminated strings.
 *
 * \param[inout] json_str The JSON string to add null terminators to.
 * \param[in]    obj_tok  Tokens extracted from json_str.
 * \param[in]    num_tok  Number of tokens in obj_tok.
 *
 * \return 0 on success, JSMN_ERROR_INVAL if string ends prematurely.
 *
 * \warning Resulting string can no longer be used as a valid json.
 ****************************************************************************/
int json_null_terminate(char *json_str, const jsmntok_t *obj_tok, int num_tok);

/************************************************************************//**
 * \brief Determines if pointed token corresponds to a key in a key:value
 * pair.
 *
 * \param[in] json_tok JSON token array.
 * \param[in] obj_idx  Index of the token to analyse.
 * \param[in] num_tok  Number of tokens in json_tok.
 *
 * \return true if token is a key token. false otherwise.
 ****************************************************************************/
bool json_is_key(const jsmntok_t *json_tok, int obj_idx, int num_tok);

/************************************************************************//**
 * \brief Return the index of the next item. This function only works
 * properly when specified token is of JSMN_OBJECT type.
 *
 * \param[in] obj_tok    JSON token array.
 * \param[in] obj_idx    Index of the token to analyse.
 * \param[in] parent_idx Index of the parent object or array token.
 * \param[in] num_tok    Number of tokens in obj_tok.
 *
 * \return The index of the next item on success, of JSMN_ERROR_PART if there
 * are no more items available in the parent object.
 ****************************************************************************/
int json_object_next(const jsmntok_t *obj_tok, int obj_idx, int parent_idx,
		int num_tok);

/************************************************************************//**
 * \brief Return the index of the next key in the current JSON level.
 *
 * \param[in] json_tok   JSON token array.
 * \param[in] obj_idx    Index of the token to analyse.
 * \param[in] parent_idx Index of the parent object or array token.
 * \param[in] num_tok    Number of tokens in obj_tok.
 *
 * \return The index of the next key item on success, of JSMN_ERROR_PART if
 * there are no more key items available in the parent object.
 ****************************************************************************/
int json_key_next(const jsmntok_t *json_tok, int obj_idx, int parent_idx,
		int num_tok);

/************************************************************************//**
 * \brief Search for a given key in the current JSON level.
 *
 * \param[in] key        key string to search.
 * \param[in] json_str   JSON string.
 * \param[in] json_tok   JSON token array.
 * \param[in] obj_idx    Index of the token to analyse.
 * \param[in] parent_idx Index of the parent object or array token.
 * \param[in] num_tok    Number of tokens in obj_tok.
 *
 * \return The index of the value token on success, or JSMN_ERROR_PART if
 * the specified key was not found in parent object.
 * \note The returned value on success is the index of the value token, it
 * is NOT the index of the found key token.
 ****************************************************************************/
int json_key_search(const char *key, const char *json_str,
		const jsmntok_t *json_tok, int obj_idx, int parent_idx,
		int num_tok);

/************************************************************************//**
 * \brief Obtains the boolean value corresponding to the indicated token.
 *
 * \param[in] json_str   JSON string.
 * \param[in] json_tok   JSON token array extracted from json_str.
 * \param[in] obj_idx    Index of the token to analyse.
 *
 * \return 1 if true, 0 if false, less than 0 if token is not a proper boolean.
 ****************************************************************************/
int json_bool_get(const char *json_str, const jsmntok_t *json_tok, int obj_idx);

#endif // MODULE_MEGAWIFI

#endif /*_JSON_H_*/

 /** \} */

