/************************************************************************//**
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

#include "config.h"
#include "types.h"
#include "string.h"


#if (MODULE_MEGAWIFI != 0)

#include "ext/mw/jsmn.h"
#include "ext/mw/json.h"

int json_null_terminate(char *json_str, const jsmntok_t *json_tok, int num_tok)
{
	int i;

	if (JSMN_OBJECT != json_tok->type) {
		return JSMN_ERROR_INVAL;
	}

	for (i = 0; i < num_tok && json_tok[i].end <= json_tok[0].end; i++) {
		json_str[json_tok[i].end] = '\0';
	}

	return 0;
}

bool json_is_key(const jsmntok_t *json_tok, int obj_idx, int num_tok)
{
	// A key token is a string type one, followed by an object, an array
	// or any other type with size 0
	// NOTE: an alternative implementation could search for the ':'
	// character after the end member of the token.
	int i = obj_idx;
	jsmntype_t type;

	if (num_tok < (i + 2)) {
		return FALSE;
	}

	// Check if current token type is string
	type = json_tok[i].type;
	if (type != JSMN_STRING) {
		return FALSE;
	}

	// Examine if next token has no children or is objet/array
	i++;
	type = json_tok[i].type;
	if (json_tok[i].size && type != JSMN_OBJECT && type != JSMN_ARRAY) {
		return FALSE;
	}

	return TRUE;
}

int json_object_next(const jsmntok_t *json_tok, int obj_idx, int parent_idx,
		int num_tok)
{
	const int limit = json_tok[parent_idx].end;
	int i = obj_idx;

	for (i = obj_idx; i < num_tok && json_tok[i].start <= limit &&
			json_tok[i].start < json_tok[obj_idx].end; i++);

	if (i >= num_tok || json_tok[i].start > limit) {
		return JSMN_ERROR_PART;
	}

	return i;
}

int json_item_next(const jsmntok_t *json_tok, int obj_idx, int parent_idx,
		int num_tok)
{
	int i = obj_idx;
	const jsmntype_t type = json_tok[i].type;

	if (JSMN_STRING == type || JSMN_PRIMITIVE == type) {
		i++;
		if (i >= num_tok ||
				json_tok[i].start > json_tok[parent_idx].end) {
			i = JSMN_ERROR_PART;
		}
	} else {
		i = json_object_next(json_tok, obj_idx, parent_idx, num_tok);
	}

	return i;
}

int json_key_next(const jsmntok_t *json_tok, int obj_idx, int parent_idx,
		int num_tok)
{
	int i = obj_idx;

	if (json_is_key(json_tok, i, num_tok)) {
		i++;
	}
	i = json_item_next(json_tok, i, parent_idx, num_tok);

	return i;
}

int json_key_search(const char *key, const char *json_str,
		const jsmntok_t *json_tok, int obj_idx, int parent_idx,
		int num_tok)
{
	int i = obj_idx;

	while (i > 0 && strcmp(key, &json_str[json_tok[i].start])) {
		i = json_key_next(json_tok, i, parent_idx, num_tok);
	}
	if (i >=0) {
		i++;	// To point to the value, not the key
	}

	return i;
}

int json_bool_get(const char *json_str, const jsmntok_t *json_tok, int obj_idx)
{
	const jsmntok_t * const tok = &json_tok[obj_idx];
	const char * const val = &json_str[tok->start];

	if (JSMN_PRIMITIVE != tok->type) {
		return JSMN_ERROR_INVAL;
	}

	// Lazy evaluation of only the first character
	switch (val[0]) {
		case 't':
			return 1;

		case 'f':
			return 0;

		default:
			return -1;
	}
}

#endif // MODULE_MEGAWIFI