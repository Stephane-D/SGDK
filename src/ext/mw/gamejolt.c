/************************************************************************//**
 * \brief GameJolt Game API implementation for MegaWiFi.
 *
 * \author Jesus Alonso (doragasu)
 * \date 2020
 ****************************************************************************/
#include "config.h"
#include "types.h"
#include "string.h"
#include "memory.h"


#if (MODULE_MEGAWIFI != 0)

#include "ext/mw/megawifi.h"
#include "ext/mw/gamejolt.h"

// I miss having stding.h sooooooo much
#ifndef INT32_MAX
// 2^31 - 1
#define INT32_MAX 2147483647
#endif

// Macro to fill optional parameters for requests
#define FILL_OPTION(key, value, index, item) \
	if (item) { \
		key[index] = #item; \
		value[index] = item; \
		index += 1; \
	}

static const char * const trophy_strings[] = {
	"Bronze", "Silver", "Gold", "Platinum", "Unknown"
};

static const char * const operation_strings[GJ_OP_MAX] = {
	"add", "subtract", "multiply", "divide", "append", "prepend"
};

enum boolean {
	BOOL_ERROR = -1,
	BOOL_FALSE =  0,
	BOOL_TRUE  =  1
};

struct {
	char *buf;
	uint16_t buf_len;
	uint16_t tout_frames;
	char username[33];
	char user_token[33];
	enum gj_error error;
} gj = {};

// memcmp not available on SGDK
static int8_t _memcmp(const char *m1, const char *m2, int16_t len)
{
	int8_t dif = 0;
	for (int16_t i = 0; i < len && !dif; i++) {
		dif = m1[i] - m2[i];
	}

	return dif;
}

// strchr not available on SGDK
static char *_strchr(const char *s, int c)
{
	while (*s) {
		if (*s == (char)c) {
			return (char*)s;
		}
		s++;
	}

	return NULL;
}

// If there is a match, *value is set to the match, and the pointer to the next
// line is returned. If the match is on last line, NULL is returned. If there
// is no match, or there is an error, *value is set to NULL and NULL is
// returned.
static char *val_get(char *line, const char *key, char **value)
{
	int len = strlen(key);
	char *val = NULL;
	char *endval = NULL;

	if (!line) {
		goto out;
	}
	if (_memcmp(line, key, len) || ':' != line[len] ||
			'"' != line[len + 1]) {
		goto out;
	}

	val = line + len + 2;
	endval = _strchr(val, '"');
	if (!endval) {
		val = NULL;
		goto out;
	}
	*endval = '\0';
	endval++;
	if ('\r' == endval[0] && '\n' == endval[1]) {
		endval += 2;
	} else {
		endval = NULL;
	}

out:
	*value = val;
	return endval;
}

static char *key_bool_get(char *line, const char *key, enum boolean *result)
{
	char *next;
	char *val;

	*result = BOOL_ERROR;
	next = val_get(line, key, &val);

	if (!val) {
		goto out;
	}

	if (!strcmp(val, "false")) {
		*result = BOOL_FALSE;
	} else if (!strcmp(val, "true")) {
		*result = BOOL_TRUE;
	}

out:
	return next;
}

bool gj_init(const char *endpoint, const char *game_id, const char *private_key,
		const char *username, const char *user_token, char *reply_buf,
		uint16_t buf_len, uint16_t tout_frames)
{
	const char *key[2] = {"game_id","format"};
	const char *value[2] = {game_id,"keypair"};

	gj.buf = reply_buf;
	gj.buf_len = buf_len;
	gj.error = GJ_ERR_NONE;

	if (mw_ga_endpoint_set(endpoint, private_key) ||
			mw_ga_key_value_add(key, value, 2)) {
		gj.error = GJ_ERR_REQUEST;
		return TRUE;
	}

	gj.tout_frames = tout_frames;
	strncpy(gj.username, username, 32);
	gj.username[32] = '\0';
	strncpy(gj.user_token, user_token, 32);
	gj.user_token[32] = '\0';

	return FALSE;
}

char *gj_recv(uint32_t *len, uint16_t tout_frames)
{
	uint16_t pos = 0;
	int16_t buf_len;
	uint8_t ch = MW_HTTP_CH;

	// Note:
	// Reception from GameJolt is a bit problematic on embedded devices,
	// because data is sent using chunked transfers, and esp_http_client
	// does not allow retrieving in advance the length of each data chunk.
	if (*len > (uint32_t)(gj.buf_len - 1)) {
		return NULL;
	}

	while (pos < *len) {
		buf_len = gj.buf_len - pos;
		if (MW_ERR_NONE != mw_recv_sync(&ch, gj.buf + pos,
					&buf_len, tout_frames)) {
			return NULL;
		}
		if (0 == buf_len) {
			// Server closed the connection
			break;
		}
		pos += buf_len;
	}

	gj.buf[pos++] = '\0';
	*len = pos;

	return gj.buf;
}

enum gj_error gj_get_error(void)
{
	return gj.error;
}

char *gj_request(const char **path, uint8_t num_paths, const char **key,
		const char **value, uint8_t num_kv_pairs, uint32_t *out_len)
{
	char *reply;
	int status;

	gj.error = GJ_ERR_NONE;
	status = mw_ga_request(MW_HTTP_METHOD_GET, path, num_paths, key,
			value, num_kv_pairs, out_len, gj.tout_frames);

	if (status < 100) {
		gj.error = GJ_ERR_REQUEST;
		return NULL;
	}
	if (status < 200 || status >= 300) {
		gj.error = status;
		return NULL;
	}
	// If chunked response, limit to the buffer length minus 1
	// character for proper string termination
	if (INT32_MAX == *out_len) {
		*out_len = gj.buf_len - 1U;
	}

	reply = gj_recv(out_len, gj.tout_frames);
	if (reply) {
		enum boolean success;
		char *aux = key_bool_get(reply, "success", &success);
		if (success != BOOL_TRUE) {
			gj.error = GJ_ERR_RESPONSE;
			return NULL;
		}
		*out_len -= aux - reply;
		reply = aux;
	} else {
		gj.error = GJ_ERR_RECEPTION;
	}
	return reply;
}

char *gj_trophies_fetch(bool achieved, const char *trophy_id)
{
	const char *path = "trophies";
	const char *key[4] = {"username", "user_token"};
	const char *val[4] = {gj.username, gj.user_token};
	uint8_t kv_idx = 2;
	uint32_t reply_len;

	if (achieved) {
		key[kv_idx] = "achieved";
		val[kv_idx] = "true";
		kv_idx++;
	}
	FILL_OPTION(key, val, kv_idx, trophy_id);

	return gj_request(&path, 1, key, val, kv_idx, &reply_len);
}

static enum gj_trophy_difficulty get_trophy(const char *difficulty_str)
{
	enum gj_trophy_difficulty difficulty = GJ_TROPHY_TYPE_UNKNOWN;

	for (int i = 0; i < GJ_TROPHY_TYPE_UNKNOWN; i++) {
		if (!strcmp(difficulty_str, trophy_strings[i])) {
			difficulty = i;
			break;
		}
	}

	return difficulty;
}

static char *decode_string(char *data, const char *key, char **output)
{
	data = val_get(data, key, output);
	if (!*output) {
		gj.error = GJ_ERR_PARSE;
		return NULL;
	}

	return data;
}

static char *decode_trophy_difficulty(char *data, const char *key,
		enum gj_trophy_difficulty *output)
{
	char *value;

	data = decode_string(data, key, &value);
	if (!value) {
		return NULL;
	}
	*output = get_trophy(value);
	if (GJ_TROPHY_TYPE_UNKNOWN == *output) {
		gj.error = GJ_ERR_PARSE;
		return NULL;
	}

	return data;
}

static char *decode_bool_num(char *data, const char *key, bool *output)
{
	char *value;

	data = decode_string(data, key, &value);
	if (!data) {
		return NULL;
	}

	if (!strcmp(value, "0")) {
		*output = BOOL_FALSE;
	} else if (!strcmp(value, "1")) {
		*output = BOOL_TRUE;
	} else {
		*output = BOOL_ERROR;
		gj.error = GJ_ERR_PARSE;
		data = NULL;
	}

	return data;
}

// To use this decoder macro, you need the following variables to be declared:
// - pos: Position of the data to decode (char*)
// - output: Struct of the corresponding data type to decode
#define X_AS_DECODER(field, decoder, type) \
	pos = decode_ ## decoder(pos, #field, &output->field); \
	if (!pos) { return NULL; }

char *gj_trophy_get_next(char *pos, struct gj_trophy *output)
{
	if (!pos || !pos[0]) {
		gj.error = GJ_ERR_PARAM;
		return NULL;
	}

	GJ_TROPHY_RESPONSE_TABLE(X_AS_DECODER);

	if ('\0' == output->description[0]) {
		output->secret = TRUE;
	} else {
		output->secret = FALSE;
	}

	return pos;
}

const char *gj_trophy_difficulty_str(enum gj_trophy_difficulty difficulty)
{
	if (difficulty < 0 || difficulty > GJ_TROPHY_TYPE_UNKNOWN) {
		difficulty = GJ_TROPHY_TYPE_UNKNOWN;
	}

	return trophy_strings[difficulty];
}

bool gj_trophy_add_achieved(const char *trophy_id)
{
	const char *path[4] = {"trophies", "add-achieved"};
	const char *key[3] = {"username", "user_token", "trophy_id"};
	const char *val[3] = {gj.username, gj.user_token, trophy_id};
	uint32_t reply_len;

	if (!trophy_id) {
		gj.error = GJ_ERR_PARAM;
		return NULL;
	}

	return !gj_request(path, 2, key, val, 3, &reply_len);
}

bool gj_trophy_remove_achieved(const char *trophy_id)
{
	const char *path[2] = {"trophies", "remove-achieved"};
	const char *key[3] = {"username", "user_token", "trophy_id"};
	const char *val[3] = {gj.username, gj.user_token, trophy_id};
	uint32_t reply_len;

	if (!trophy_id) {
		gj.error = GJ_ERR_PARAM;
		return NULL;
	}

	return !gj_request(path, 2, key, val, 3, &reply_len);
}

bool gj_time(struct gj_time *output)
{
	const char *path = "time";
	uint32_t len;
	char *pos = gj_request(&path, 1, NULL, NULL, 0, &len);

	GJ_TIME_RESPONSE_TABLE(X_AS_DECODER);

	return !pos;
}


bool gj_scores_add(const char *score, const char *sort, const char *table_id,
		const char *guest, const char *extra_data)
{
	const char *path[2] = {"scores", "add"};
	const char *key[6] = {"score", "sort"};
	const char *val[6] = {score, sort};
	int kv_idx = 2;
	uint32_t reply_len;

	if (!score || !sort) {
		gj.error = GJ_ERR_PARAM;
		return TRUE;
	}
	if (!guest) {
		key[kv_idx] = "username";
		val[kv_idx++] = gj.username;
		key[kv_idx] = "user_token";
		val[kv_idx++] = gj.user_token;
	}
	FILL_OPTION(key, val, kv_idx, table_id);
	FILL_OPTION(key, val, kv_idx, guest);
	FILL_OPTION(key, val, kv_idx, extra_data);

	return !gj_request(path, 2, key, val, kv_idx, &reply_len);
}

char *gj_scores_fetch(const char *limit, const char *table_id,
		const char *guest, const char *better_than,
		const char *worse_than, bool only_user)
{
	const char *path = "scores";
	const char *key[6] = {NULL};
	const char *val[6] = {NULL};
	int kv_idx = 0;
	uint32_t reply_len;

	if (!guest && only_user) {
		key[kv_idx] = "username";
		val[kv_idx++] = gj.username;
		key[kv_idx] = "user_token";
		val[kv_idx++] = gj.user_token;
	}
	FILL_OPTION(key, val, kv_idx, limit);
	FILL_OPTION(key, val, kv_idx, table_id);
	FILL_OPTION(key, val, kv_idx, guest);
	FILL_OPTION(key, val, kv_idx, better_than);
	FILL_OPTION(key, val, kv_idx, worse_than);

	return gj_request(&path, 1, key, val, kv_idx, &reply_len);
}

char *gj_score_get_next(char *pos, struct gj_score *output)
{
	if (!pos || !pos[0]) {
		gj.error = GJ_ERR_PARAM;
		return NULL;
	}

	GJ_SCORE_RESPONSE_TABLE(X_AS_DECODER);

	return pos;
}

char *gj_scores_tables_fetch(void)
{
	const char *path[2] = {"scores", "tables"};
	uint32_t reply_len;

	return gj_request(path, 2, NULL, NULL, 0, &reply_len);
}

char *gj_score_table_get_next(char *pos, struct gj_score_table *output)
{
	if (!pos || !pos[0]) {
		gj.error = GJ_ERR_PARAM;
		return NULL;
	}

	GJ_SCORE_TABLE_RESPONSE_TABLE(X_AS_DECODER);

	return pos;
}

char *gj_scores_get_rank(const char *sort, const char *table_id)
{
	const char *path[2] = {"scores", "get-rank"};
	const char *key[2] = {"sort"};
	const char *val[2] = {sort};
	int kv_idx = 1;
	uint32_t reply_len;
	char *rank = NULL;
	char *data;

	if (!sort) {
		gj.error = GJ_ERR_PARAM;
		return NULL;
	}

	FILL_OPTION(key, val, kv_idx, table_id);

	data = gj_request(path, 2, key, val, kv_idx, &reply_len);

	if (!data) {
		return NULL;
	}
	data = decode_string(data, "rank", &rank);
	if (!data) {
		return NULL;
	}

	return rank;
}

bool gj_data_store_set(const char *key, const char *data, bool user_store)
{
	const char *path[2] = {"data-store", "set"};
	const char *key_arr[4] = {"key", "data"};
	const char *val_arr[4] = {key, data};
	int kv_idx = 2;
	uint32_t reply_len;

	if (!key || !data) {
		gj.error = GJ_ERR_PARAM;
		return TRUE;
	}

	if (user_store) {
		key_arr[kv_idx] = "username";
		val_arr[kv_idx++] = gj.username;
		key_arr[kv_idx] = "user_token";
		val_arr[kv_idx++] = gj.user_token;
	}

	return !gj_request(path, 2, key_arr, val_arr, kv_idx, &reply_len);
}

char *gj_data_store_keys_fetch(const char *pattern, bool user_store)
{
	const char *path[2] = {"data-store", "get-keys"};
	const char *key[3] = {0};
	const char *val[3] = {0};
	int kv_idx = 0;
	uint32_t reply_len;
	char *result;

	FILL_OPTION(key, val, kv_idx, pattern);
	if (user_store) {
		key[kv_idx] = "username";
		val[kv_idx++] = gj.username;
		key[kv_idx] = "user_token";
		val[kv_idx++] = gj.user_token;
	}

	result = gj_request(path, 2, key, val, kv_idx, &reply_len);
	if (!result) {
		return NULL;
	}
	// On empty list, "keys" is returned instead of a "key" array.
	if (0 == _memcmp(result, "keys:", 5)) {
		*result = '\0';
	}
	return result;
}

char *gj_data_store_key_next(char *pos, char **output)
{
	return decode_string(pos, "key", output);
}

char *data_store_fetch(const char *key, bool user_store)
{
	const char *path = "data-store";
	const char *key_arr[3] = {"key"};
	const char *val_arr[3] = {key};
	int kv_idx = 1;
	uint32_t reply_len;
	char *result;

	if (!key) {
		gj.error = GJ_ERR_PARAM;
		return NULL;
	}

	if (user_store) {
		key_arr[kv_idx] = "username";
		val_arr[kv_idx++] = gj.username;
		key_arr[kv_idx] = "user_token";
		val_arr[kv_idx++] = gj.user_token;
	}

	result = gj_request(&path, 1, key_arr, val_arr, kv_idx, &reply_len);
	if (!result || !decode_string(result, "data", &result)) {
		return NULL;
	}

	return result;
}

char *gj_data_store_update(const char *key,
		enum gj_data_store_update_operation operation,
		const char *value, bool user_store)
{
	const char *path[2] = {"data-store", "update"};
	const char *key_arr[5] = {"key", "value", "operation"};
	const char *val_arr[5] = {key, value};
	int kv_idx = 2;
	uint32_t reply_len;
	char *result;

	if (!key || !value || operation < 0 || operation >= GJ_OP_MAX) {
		gj.error = GJ_ERR_PARAM;
		return NULL;
	}

	val_arr[kv_idx++] = operation_strings[operation];

	if (user_store) {
		key_arr[kv_idx] = "username";
		val_arr[kv_idx++] = gj.username;
		key_arr[kv_idx] = "user_token";
		val_arr[kv_idx++] = gj.user_token;
	}

	result = gj_request(path, 2, key_arr, val_arr, kv_idx, &reply_len);
	if (!result || !decode_string(result, "data", &result)) {
		return NULL;
	}

	return result;
}

bool gj_data_store_remove(const char *key, bool user_store)
{
	const char *path[2] = {"data-store", "remove"};
	const char *key_arr[3] = {"key"};
	const char *val_arr[3] = {key};
	int kv_idx = 1;
	uint32_t reply_len;

	if (!key) {
		gj.error = GJ_ERR_PARAM;
		return NULL;
	}

	if (user_store) {
		key_arr[kv_idx] = "username";
		val_arr[kv_idx++] = gj.username;
		key_arr[kv_idx] = "user_token";
		val_arr[kv_idx++] = gj.user_token;
	}

	return !gj_request(path, 2, key_arr, val_arr, kv_idx, &reply_len);
}

bool gj_sessions_open(void)
{
	const char *path[2] = {"sessions", "open"};
	const char *key[2] = {"username", "user_token"};
	const char *val[2] = {gj.username, gj.user_token};
	uint32_t reply_len;

	return !gj_request(path, 2, key, val, 2, &reply_len);
}

bool gj_sessions_ping(bool active)
{
	const char *path[2] = {"sessions", "ping"};
	const char *key[3] = {"username", "user_token", "status"};
	const char *val[3] = {gj.username, gj.user_token};
	uint32_t reply_len;

	val[2] = active ? "active" : "idle";

	return !gj_request(path, 2, key, val, 3, &reply_len);
}

bool gj_sessions_check(const char *username, const char *user_token)
{
	const char *path[2] = {"sessions", "check"};
	const char *key[2] = {"username", "user_token"};
	const char *val[2];
	uint32_t reply_len;
	bool open_session = FALSE;
	char *result;

	if (!username || !user_token) {
		key[0] = gj.username;
		key[1] = gj.user_token;
	} else {
		key[0] = username;
		key[1] = user_token;
	}

	result = gj_request(path, 2, key, val, 2, &reply_len);
	if (result) {
		open_session = TRUE;
	} else {
		if (GJ_ERR_RESPONSE == gj.error) {
			gj.error = GJ_ERR_NONE;
		}
	}

	return open_session;
}

bool gj_sessions_close(void)
{
	const char *path[2] = {"sessions", "close"};
	const char *key[2] = {"username", "user_token"};
	const char *val[2] = {gj.username, gj.user_token};
	uint32_t reply_len;

	return !gj_request(path, 2, key, val, 2, &reply_len);
}

char *gj_users_fetch(const char *username, const char *user_id)
{
	const char *path = "users";
	const char *key;
	const char *val;
	uint32_t reply_len;

	if (username) {
		key = "username";
		val = username;
	} else if (user_id) {
		key = "user_id";
		val = user_id;
	} else {
		gj.error = GJ_ERR_PARAM;
		return NULL;
	}

	return gj_request(&path, 1, &key, &val, 1, &reply_len);

}

char *gj_user_get_next(char *pos, struct gj_user *output)
{
	if (!pos || !pos[0]) {
		gj.error = GJ_ERR_PARAM;
		return NULL;
	}

	GJ_USER_RESPONSE_TABLE(X_AS_DECODER);

	return pos;
}


bool gj_users_auth(void)
{
	const char *path[2] = {"users", "auth"};
	const char *key[2] = {"username", "user_token"};
	const char *val[2] = {gj.username, gj.user_token};
	uint32_t reply_len;
	bool user_auth = FALSE;
	char *result;

	result = gj_request(path, 2, key, val, 2, &reply_len);
	if (result) {
		user_auth = TRUE;
	} else {
		if (GJ_ERR_RESPONSE == gj.error) {
			gj.error = GJ_ERR_NONE;
		}
	}

	return user_auth;
}

char *gj_friends_fetch(void)
{
	const char *path = "friends";
	const char *key[2] = {"username", "user_token"};
	const char *val[2] = {gj.username, gj.user_token};
	uint32_t reply_len;

	return gj_request(&path, 1, key, val, 2, &reply_len);
}

char *gj_friend_get_next(char *pos, char **user_id)
{
	return decode_string(pos, "friend_id", user_id);
}


#endif // MODULE_MEGAWIFI
