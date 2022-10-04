# MegaWiFi API

## Introduction

This repository contains the MegaWiFi API, to use the WiFi capabilities of the MegaWiFi cartridges.

## API documentation

The full API documentation [can be found here](https://doragasu.github.io/mw-api/doc/html/index.html).

## Building

You will need a complete Genesis/Megadrive toolchain. The sources use some C standard library calls, such as `memcpy()`, `strchr()`, etc. Thus your toolchain must include a C standard library implementation such as *newlib*. Alternatively you can use the version integrated into the awesome [SGDK](https://github.com/Stephane-D/SGDK/).

## Overview

The MegaWiFi API consists of the following modules:
* tsk: Simple multitasking implementation (one supervisor task and one user task).
* megawifi: Communications with the WiFi module and the Internet, including sockets and HTTP/HTTPS.
* mw-msg: MegaWiFi command message definitions.
* util: General purpose utility functions and macros.
* gamejolt: [Gamejolt Game API implementation](https://gamejolt.com/game-api/doc), allowing to easily add online trophies and scoreboards to your game, manage friends, sessions, etc.

The `mw-msg` module contains the message definitions for the different MegaWiFi commands and command replies. Fear not because usually you do not need to use this module, unless you are doing something pretty advanced not covered by the `megawifi` module API.

The `util` module contains general purpose functions and macros not fitting in the other modules, such as `ip_validate()` to check if a string is a valid IP address, `str_to_uint8()` to convert a string to an 8-bit number, etc. There is also a `json` module wich includes `jsmn` library along with some helper functions to parse JSON formatted strings. Please read `jsmn` documentation to learn how its tokenizer works.

Previous MegaWiFi releases also included two more modules: `mpool` (a quite limited dynamic memory allocator) and `loop` (a loop functions and timers implementation). I found most devs considered these modules confusing, so I removed them. If you use `newlib` and require dynamic memory allocation, you can implemente `_sbrk()` syscall and use the standard `malloc()` and `free()` functions. You can also use SGDK that already includes a dynamic memory allocator. As for the `loop` module, it has somehow been replaced by the `tsk` module.

### tsk module

This module implements basic multitasking capabilities. It allows setting up a task that runs in user mode, in addition to the supervisor task. For the user task to run, the supervisor task must give it some CPU time. This can be done in two ways:

* By calling the lock function `tsk_super_pend()`: this causes the user task to resume execution. User task will keep running until it calls `tsk_super_post()`, or the timeout specified in the `tsk_supper_pend()` occurs. Then supervisor task will resume execution.
* By calling `tsk_user_yield()`: this causes the user task to resume execution. User task will keep running until the next VBLANK interrupt, or until `tsk_super_post()` is called with the `force_ctx_sw` param set to `true`.

User task will not run unless the supervisor task calls either `tsk_super_pend()` or `tsk_user_yield()`. So you must make sure to call one of them in your main loop or the user task will starve. If you are using SGDK, the `tsk_user_yield()` call is done transparently inside `VDP_waitVSync()`, `VDP_waitVActive()` and `SYS_doVBlankProcess()`. So just make sure you are calling one of these in your loop and you should be ready to go!

A typical Megadrive game contains a main loop with a structure similar to this:

```C
void main()
{
	// Perform initialization
	init();

	// Infinite loop with game logic
	while(1) {
		wait_vblank();
		read_input();
		draw_screen();
		play_sound();
		game_logic();
	}
}
```

The game performs the initialization using `init()` function, and then enters an infinite loop that:

1. Waits for the vertical blanking period to begin. Inside this function `tsk_user_yield()` must be called, either directly or indirectly (if you are using SGDK, as explained above).
4. Reads controller inputs.
2. Updates the frame (scroll, sprites, tiles, etc).
3. Keeps the music and SFX playing.
5. Computes game logic, such as collision detection, player/enemy movements, etc. When this step finishes, we have all the data to draw the next frame.

The order of these elements might be slightly different, but these are the usual suspects in loop game design.

### Megawifi module

And we finally arrive to the `megawifi` module API. This API allows of course sending and receiving data to/from the Internet, along with some more functions such as:

* Scanning APs, associating and disassociating to/from them.
* Creating both client and server network sockets.
* Performing HTTP/HTTPS client requests.
* Reading and writing from/to the non-volatile flash memory in the WiFi module.
* Keeping the time and day, accurately synchronized to NTP servers.
* Generating large amounts of random numbers blazingly fast.

You can use this API the hard way (directly sending commands defined in `mw-msg`), or the easy way (through the API calls in `megawifi`). Of course the later is recommended.

Most API functions require sending and receiving data to/from the WiFi module. But the data send/reception is decoupled from the command functions: the API functions prepare the module to send/receive data, but the data is not sent/received until the `mw_process()` function is called. As the `mw_process()` function polls the WiFi module for data, it is advisable to run it as frequently as possible. The easiest way to do this, is creating a user task that continuously runs `mw_process()`:

```C
#include "mw/util.h"
#include "mw/tsk.h"
#include "mw/megawifi.h"

static void user_tsk(void)
{
	while (1) {
		mw_process();
	}
}

static void task_init(void)
{
	// Configure the user task
	tsk_user_set(user_tsk);
}
```

Using a task like this makes sending and receiving data during game idle time way easier. Just make sure you have some spare CPU time for the user task to run and call `mw_process()`. Otherwise this task will starve and no data will be sent/received!

About the API calls, basically all of them are synchronous or pseudo-synchronous, excepting the following ones, that are asynchronous and use callbacks to signal task completion:

* `mw_send()`: Send data to the other socket end.
* `mw_recv()`: Receive data from the other socket end.
* `mw_cmd_send()`: Send a command to the WiFi module.
* `mw_cmd_recv()`: Receive a command reply from the WiFi module.

Usually `mw_cmd_send()` and `mw_cmd_recv()` are not needed unless you decide to go down the hard path (using `mw-msg` to build commands yourself). For sending/receiving data, it's up to you using the asynchronous `mw_send()` and `mw_recv()` or their pseudo-synchronous counterparts `mw_send_sync()` and `mw_recv_sync()`.

To save precious RAM, command functions reuse the same buffer. Thus when a command reply is obtained, you have to copy the needed data from the buffer before issuing another command. Otherwise the data in the previously received buffer will be lost.

## Putting all together

In this section several examples explaining how to code typical tasks are presented.

### Connection configuration

MegaWiFi modules have 3 configuration slots, allowing to store 3 different network configurations. The configuration parameters are:

* Access point configuration (SSID, password), using `mw_ap_cfg_set()`. This usually requires a previous AP scan using `mw_ap_scan()` and cycling through scan results using `mw_ap_fill_next()`.
* IP configuration, using `mw_ip_cfg_set()`. Both automatic (DHCP) and manual configurations are supported.

The good news is that you do not need to code the connection configuration, you can use the [wflash bootloader](https://github.com/doragasu/mw-wflash/) to configure the network. As the configuration is stored inside the module, you can use it from your game, even if you delete the wflash bootloader ROM from the MegaWiFi cartridge.

### Program initialization

Basically you have to initialize megawifi and the game loop as explained before. You also have to create a user task to run `mw_process()`. The code below shows how to do this, and also how to detect if the WiFi module is installed, along with its firmware version.

```C
#include "mw/util.h"
#include "mw/tsk.h"
#include "mw/megawifi.h"

// Length of the wflash buffer
#define MW_BUFLEN	1440

// TCP port to use (set to Megadrive release year ;-)
#define MW_CH_PORT 	1989

// Command buffer
static char cmd_buf[MW_BUFLEN];

// Runs mw_process() during idle time
static void user_tsk(void)
{
	while (1) {
		mw_process();
	}
}

// MegaWiFi initialization
static void megawifi_init(void)
{
	uint8_t ver_major = 0, ver_minor = 0;
	char *variant = NULL;
	enum mw_err err;

	// Initialize MegaWiFi
	mw_init(cmd_buf, MW_BUFLEN);

	// Try detecting the module
	err = mw_detect(&ver_major, &ver_minor, &variant);

	if (MW_ERR_NONE != err) {
		// Megawifi cart not found!
		// [...]
	} else {
		// MegaWiFi found!
		// [...]
	}
}

// Run the game loop once per frame
static void game_loop(void)
{
	// Make sure this yields spare CPU time to user task
	wait_vblank();
	read_input();
	draw_screen();
	play_sound();
	game_logic();
}

// Global initialization
static void init(void)
{
	// Initialize hardware and game
	// [...]
	// Configure user task
	tsk_user_set(user_tsk);
	// Initialize MegaWiFi
	megawifi_init();
}

/// Entry point
void main()
{
	// Initialization
	init();

	while (1) {
		game_loop();
	}
}
```

### Associating to an AP

Once configured, associating to an AP is easy. Just call `mw_ap_assoc()` with the desired configuration slot, and the module will start the process. You can wait until the association is successful or fails (because of timeout) by calling `mw_ap_assoc_wait()`. The following code tries to associate to an AP during 30 seconds (*fps* must be set previously to 60 on NTSC machines or 50 on PAL machines).

```C
	enum mw_err err;

	err = mw_ap_assoc(slot);
	if (MW_ERR_NONE == err) {
		err = mw_ap_assoc_wait(30 * fps);
	}
	if (MW_ERR_NONE == err) {
		// Association succeeded
	} else {
		// Association failed
	}
```

Once association has succeeded, you can try connecting to a server, or creating a server socket. DNS service will also start automatically after associating to the AP, but it takes a little more time.

### Connecting to a TCP server

Connecting to a server is straightforward: just call `mw_tcp_connect()` with the channel to use, the destination address (both IPv4 addresses and domain names are supported), the destination port, and optionally the origin port (if NULL, it will be automatically set):

```C
	enum mw_err err;

	err = mw_tcp_connect(1, "www.example.com", "443", NULL);
	if (MW_ERR_NONE == err) {
		// Connection succeeded
	} else {
		// Connection failed
	}
```

Once connected, you can start sending and receiving data. When no longer needed, remember to close the connection with `mw_tcp_disconnect()`. The channel number must be from 1 to `LSD_MAX_CH - 1` (usually 2). The used channel number will be passed to all the calls relative to the connected socket (think about it like a socket number).

### Creating a TCP server socket

Creating a TCP server socket requires binding it to a port, using `mw_tcp_bind()`. After this, MegaWiFi will automatically accept any incoming connection on this port. You can check when the connection has been established by calling `mw_sock_conn_wait()`:

```C
	enum mw_err err;

	err = mw_tcp_bind(1, 1989);
	if (MW_ERR_NONE == err) {
		// Wait up to an hour for an incoming connection
		err = mw_sock_conn_wait(1, 60 * 60 * fps);
	}
	if (MW_ERR_NONE == err) {
		// Incoming connection established
	} else {
		// Timeout, no connection established
	}
```

### Sending data

You can send data once a connection has been established. The easiest way is using the synchronous variant, but as it suspends the execution of the calling function until data is sent, sometimes the asynchronous version is more convenient. The following code shows how to send *data* buffer of *data_length* length using channel 1, with a two second timeout:

```C
	enum mw_err err;

	err = mw_send_sync(1, data, data_length, 2 * fps);
	if (MW_ERR_NONE == err) {
		// Data sent
	} else {
		// Timeout, data was not sent
	}
```

The same data can be sent this way using the asynchronous API:

```C
void send_complete_cb(enum lsd_status stat, void *ctx)
{
	UNUSED_PARAM(ctx);

	if (LSD_STAT_COMPLETE == stat) {
		// Data successfully sent
	} else {
		// Sending data failed
	}
}

void send_example(void)
{
	enum lsd_status stat;

	stat = mw_send(1, data, data_length, NULL, send_complete_cb);
	if (stat < 0) {
		// Sending failed
	}
}
```

When using the asynchronous API, sometimes you do not need confirmation about when data has been sent. In that case, you do not need to use a completion callback, you can call `mw_send()` with this parameter set to NULL.

### Receiving data

You can receive data once a connection has been established. The easiest way is using the synchronous variant, but as it suspends the execution of the calling function until data is received, sometimes the asynchronous version is more convenient. The following code shows how to receive *data* buffer of *buf_length* maximum length using channel 1, with a 30 second timeout:

```C
	enum mw_err err;

	err = mw_recv_sync(1, data, &buf_length, 30 * fps);
	if (MW_ERR_NONE == err) {
		// Data received
	} else {
		// Failed to receive data
	}
```

Note that when the function successfully returns, *buf_length* contains the number of bytes received.

The same data can be received this way using the asynchronous API:

```C
void recv_complete_cb(enum lsd_status stat, uint8_t ch, char *data,
			uint16_t len, void *ctx)
{
	UNUSED_PARAM(ctx);

	if (LSD_STAT_COMPLETE == stat) {
		// Data successfully received
	} else {
		// Data reception failed
	}
}

void recv_example(void)
{
	enum lsd_status stat;

	stat = mw_recv(data, data_length, NULL, recv_complete_cb);
	if (stat < 0) {
		// Reception failed
	} else {
		// Data will be received by mw_process()
	}
}
```

### Performing an HTTP/HTTPS request

`megawifi` module allows performing HTTP and HTTPS requests in a simple way. HTTP and HTTPS use the same API, the only difference is that when using HTTPS, you can set an SSL certificate for the server identity to be verified. You can skip this step when using plain HTTP. Performing an HTTPS request requires the following steps. Some of them are optional and depend on the use case.

1. (**Optional**) Set the SSL certificate. You can skip this step when using HTTP, or if you do not need to verify the server identity. You can retrieve the x509 hash of the currently stored certificate by calling `mw_http_cert_query()`. To set a different certificate, call `mw_http_cert_set()`. Once set, the certificate is stored on the non volatile memory, and will remain until replaced with a new one. Only one certificate can be stored at a time. Note you should not use this function unless required, because as it writes to Flash memory, it can wear the storage if used too often.
2. Set the URL (e.g. https://www.example.com). Use `mw_http_url_set()` for this purpose.
3. Set the HTTP method. Most commonly used ones are `MW_HTTP_METHOD_GET` and `MW_HTTP_METHOD_POST`. Use `mw_http_method_set()` to set it.
4. (**Optional**) add HTTP headers to the request. Many aspects of the requests can be controlled via headers. E.g. you can define the formatting of the data you are posting by adding the header "Content-type" with the mime type "text/html", "application/json", etc., or you can add access credentials using HTTP basic authentication, bearer authentication, etc.
5. Open the connection. In this step, the HTTP or HTTPS connection is opened, and the request (including any headers added) is sent to the server. If the request contains a data payload, in this step the payload length is specified. The function `mw_http_open()` does this.
6. (**Optional**) send the request data payload (if any). This must be performed only if a payload length (greater than 0) was specified in the previous step. This is done the same way as sending data through sockets, with the `mw_send()` or `mw_send_sync()` functions, using the HTTP reserved channel (`MW_CH_HTTP`).
7. Finish the transaction. Call `mw_http_finish()` for the HTTP client to obtain the response to the request, along with its associated headers. If a response includes a data payload, its length is obtained in this step.
8. (**Optional**) if the previous step returned a reply payload length greater than 0, it must be received in this step by calling `mw_recv()` or `mw_recv_sync()` using the HTTP reserved channel (`MW_CH_HTTP`).

By looking to this list of steps, it might seem complicated to perform an HTTP request, but the steps are relatively simple, and can be easily added to some higher level functions. E.g., this code allows performing arbritrary `GET` (without payload) and `POST` (with `JSON` payload) requests:

```C
// Performs initial steps of an HTTP request
static int http_begin(enum mw_http_method type, const char *url,
		unsigned int len) {
	enum mw_err err;

	err = mw_http_url_set(url);
	if (!err) {
		err = mw_http_method_set(type);
	}
	if (!err) {
		err = mw_http_open(len);
	}

	return err;
}

// Tries to synchronously receive exactly the indicated data length
static int sync_recv(uint8_t ch, char *buf, int len, uint16_t tout_frames)
{
	int recvd = 0;
	int err = 0;
	uint8_t get_ch = ch;
	int16_t get_len;

	while (err == 0 && recvd < len) {
		get_len = len - recvd;
		err = mw_recv_sync(&get_ch, buf + recvd, &get_len, tout_frames);
		if (!err) {
			if (get_ch != ch) {
				err = -1;
			} else {
				recvd += get_len;
			}
		}
	}

	return err;
}

// Performs final steps of an HTTP request
static int http_finish(char *recv_buf, unsigned int *len)
{
	enum mw_err err;
	uint32_t content_len = 0;

	err = mw_http_finish(&content_len, MS_TO_FRAMES(60000));
	err = err >= 200 && err <= 300 ? 0 : err;
	if (content_len > b.msg_buf_len) {
		err = -1;
	}
	if (!err && content_len) {
		err = sync_recv(MW_HTTP_CH, recv_buf, content_len, 0);
	}
	if (!err && len) {
		*len = content_len;
	}

	return err;
}

/****************************************************************************
 * \brief Generic HTTP GET request without data payload.
 *
 * \param[in]  url      URL for the request.
 * \param[out] recv_buf Buffer used for HTTP response data.
 * \param[out] len      Length of the response.
 *
 * \return HTTP status code, or -1 if request was not completed.
 ****************************************************************************/
int http_get(const char *url, char *recv_buf, unsigned int *len)
{
	enum mw_err err;

	err = http_begin(MW_HTTP_METHOD_GET, url, 0);
	if (!err) {
		err = http_finish(recv_buf, len);
	}

	return err;
}

/****************************************************************************
 * \brief Generic POST request with JSON data payload.
 *
 * \param[in]    url          URL for the request.
 * \param[in]    data         Data to send in the POST data payload.
 * \param[in]    length       Length of the data to send.
 * \param[in]    content_type Content-Type HTTP header for the data payload.
 * \param[out]   recv_buf     Buffer used to receive reply data.
 * \param[inout] recv_len     On input, length of recv_buf. On output, length
 *                            of the received reply data.
 *
 * \return HTTP status code or -1 if the request was not completed.
 ****************************************************************************/
int http_post(const char *url, const char *data, int length,
		const char *content_type, char *recv_buf,
		unsigned int *recv_len)
{
	enum mw_err err;

	// If this function fails, it is not critical
	mw_http_header_add("Content-Type", content_type);

	err = http_begin(MW_HTTP_METHOD_POST, url, length);
	if (!err) {
		err = mw_send_sync(MW_HTTP_CH, data, length, 0);
	}
	if (!err) {
		err = http_finish(recv_buf, recv_len);
	}

	return err;
}
```

In case you want HTTPS, you can set a PEM formatted certificate as follows:

```C
void http_cert_set(const char *cert, int cert_len, uint32_t cert_hash)
{
	uint32_t hash = mw_http_cert_query();
	if (hash != cert_hash) {
		mw_http_cert_set(cert_hash, cert, cert_len);
	}
}
```

This function only sets the certificate if it has not been previously stored, avoiding to unnecessarily wear the Flash memory. To obtain a correct certificate hash, you can use openssl:

```
$ openssl x509 -hash in <cert_file_name> -noout
```

### Getting the date and time

MegaWiFi allows to synchronize the date and time to NTP servers. It is important to note that on console power up, the module date and time will be incorrect and should not be used. For the date and time to be synchronized, the module must be associated to an AP with Internet connectivity. Once associated, the date and time is automatically synchronized. The synchronization procedure usually takes only a few seconds, and once completed, date/time should be usable until the console is powered off.


To guess if the date and time is in sync, you can check the `dt_ok` field of `mw_msg_sys_stat` union, by calling `mw_sys_stat_get()`:

```C
	union mw_msg_sys_stat *sys_stat;

	sys_stat = mw_sys_stat_get();
	if (sys_stat && sys_stat->dt_ok) {
		// Date and time syncrhonized
	} else {
		// Date and time **not** synchronized, or other error
	}
```

Once date and time is synchronized, you can get it, both in human readable format, and in the number of seconds elapsed since the epoch, with a single call to `mw_date_time_get()`:

```C
	char *date_time_string;
	uint32_t date_time_bin[2];

	date_time_string = mw_date_time_get(date_time_bin);
```

For the date/time to work properly, the timezone and NTP servers must be properly set. This is done calling `mw_sntp_cfg_set()`, and configuration is stored in non volatile flash to survive power cycles, so you should do this only once. As an example, you can set the time configuration for the eastern Europe timezone (GMT-1 when not in dailight time) as follows:

```C
	enum mw_err err;
	const char *timezone = "GMT-1";
	const char *ntp_serv[] = {
		"0.pool.ntp.org",
		"1.pool.ntp.org",
		"2.pool.ntp.org"
	};

	err = mw_sntp_cfg_set(timezone, ntp_serv);
```

### Setting and getting gamertag information

MegaWiFi API allows to store and retrieve up to 3 gamertags. The gamertag information is contained in the *mw_gamertag* structure. This structure holds the gamertag unique identifier, nickname, security credentials (password) and a 32x48 avatar (tile information and palette). This example shows how to set a gamertag (excepting the graphics data):

```C
void gamertag_set(int slot, int id, const char *name,
		const char *security, const char *tagline)
{
	struct mw_gamertag gamertag = {};
	struct mw_err err;

	gamertag.id = id;
	strcpy(gamertag.nickname, name);
	strcpy(gamertag.security, security);
	strcpy(gamertag.tagline, tagline);

	err = mw_gamertag_set(slot, &gamertag);
	if (MW_ERR_NONE == err) {
		// Gamertag set successfully
	} else {
		// Setting gamertag failed
	}
}
```

To read the gamertag, just call `mw_gamertag_get()` function, and the information corresponding to the requested slot will be returned:

```C
	struct mw_gamertag *gamertag = mw_gamertag_get(slot);

	if (!gamertag) {
		// Something went wrong
	} else {
		// Success!
	}
```

### Reading the module BSSIDs

The module has two network interfaces, each one with its unique BSSID (MAC address). One interface is used for the station mode, while the other is for the AP mode. Each BSSID is 6-byte long. Currently the API does not allow using the AP mode, so to get the station mode BSSID you can do as follows:

```C
	uint8_t *bssid = mw_bssid_get(MW_IF_STATION);

	if (!bssid) {
		// Something went wrong
	} else {
		// Success! You can use bssid[0] through bssid[5]
	}
```

### Reading and writing to non-volatile Flash

In addition to the standard 32 megabits of Flash ROM memory connected to the Megadrive 68k bus, MegaWiFi cartridges have 16 megabits of additional flash storage, directly usable by the game. This memory is organized in 4 KiB sectors, and supports the following operations:

* Identify: call `mw_flash_id_get()` to obtain the flash memory identifiers. Usually not needed.
* Erase: call `mw_flash_sector_erase()` to erase an entire 4 KiB sector. Erased sectors will be read as `0xFF`.
* Program: call `mw_flash_write()` to write the specified data buffer to the indicated address. Prior to programming, **make sure the programmed address range is previously erased**, otherwise operation will fail.
* Read: call `mw_flash_read()` to read the specified amount of data from the indicated address.

These functions can be used e.g. for high score keeping or DLCs. When using these functions, you have to keep in mind that flash can only be erased in a 1 sector (i.e. 4 KiB) granularity, and thus if e.g. you want to keep high scores, to update one of the high scores, you will have to erase the complete sector, and write it again in its entirety.

Also keep in mind that flash memory suffers from wearing, so do not perform more writes than necessary.

### GameJolt Game API

GameJolt API is implemented on top of the HTTP APIs, so the HTTP reserved channel is used to receive data when using the GameJolt API module. The current version 1.2 is fully supported, excepting the batch function, that maybe will not be very useful on the Megadrive, because of the tight memory constraints. It is recommended you complement this documentation with the [official documentation of the API](https://gamejolt.com/game-api/doc). You will find additional details there.

The first thing you need to know is that the GameJolt API implementation for MegaWiFi has the following restrictions:

 * On initialization (`gj_init()`), the receive buffer is configured. To avoid wasting RAM, usually you will want to use the same buffer used for all other communication routines (the one specified on `mw_init()` invocation for example). When an API call returns pointers to data received from server, they point directly to different regions of this buffer. This means that before reusing the buffer again (i.e. before doing any other MegaWiFi call), you must copy all the data you want to preserve, or it will vanish before your eyes.
 * The maximum reply of the server you will be able to receive, is currently limited by the size of the buffer set on initialization. I would recommend using at least 2 KiB, because some calls can return a bunch of data (for example the `gj_trophies_fetch()` and `gj_users_fetch()` if you get data for several users in a row). It is the developer work to make sure the data will fit in the buffer (otherwise the API call will fail). For example if you create a lot of trophies for your game, you have to set the buffer length accordingly. Pay very careful attention to this, or your game online functions might fail unexpectedly.
 * The official API defines some input and output parameters as integers. But the implementation on the Megadrive uses the string representation of the integers for input parameters and output data. This has been done on purpose to avoid as much as possible conversions between strings and integers, that can be slow on the m68k side.
 * As mentioned earlier, `Batch` command is not supported.

Other than these, the API is full featured, and I'm sure you will be able to do really cool things with it!

Here are some examples on using the API. Not all the supported features are shown, but for sure you will get an idea on how to use them. As mentioned earlier, do not forget to also check the [official API documentation](https://gamejolt.com/game-api/doc).

#### Initialization

To initialize the API, you have to pass the endpoint, the game and user credentials, the buffer and the timeout to use for requests. It is recommended to let user configure the credentials in the gamertag slots, and get them from there using `mw_gamertag_get()` before calling `gj_init()`.

```C
	if (gj_init("https://api.gamejolt.com/api/game/v1_2/", GJ_GAME_ID,
				GJ_PRIV_KEY, username, user_token,
				cmd_buf, MW_BUFLEN, MS_TO_FRAMES(30000))) {
		// Handle init error
	}
```

Make sure you store the credentials in a safe place, specially the game private key.

#### Achieving a trophy

Trophies are added using the GameJolt Web UI. Once added, to make the player achieve a trophy, just call `gj_trophy_add_achieved()` with the trophy id:

```C
	if (gj_trophy_add_achieved("121457")) {
		// Something went wrong, trophy not achieved!
	}
```

#### Getting game trophies

You can get all the available trophies, the ones achieved by the player, or a single one by id. Then you have to iterate on the returned data to get the trophy data one by one:

```C
	char *trophy_data;
	struct gj_trophy trophy;

	trophy_data = gj_trophies_fetch(false, NULL);
	if (!trophy_data) {
		// Treat error
		return;
	}
	while (trophy_data && *trophy_data) {
		trophy_data = gj_trophy_get_next(trophy_data, &trophy);
		if (!trophy_data) {
			// Error formatting trophy
			return;
		}
		beautifully_print_the_trophy(&trophy);
	}
```

#### Posting a score

When posting scores, you can put them on the global game score table, or on any other additional table the developer has created for the game. You can also post them as the user, or as a guest (when allowed), and you must post both a textual representation of the score and a numeric sort value (but both can be the same). Also optionally, extra data can be added with the score:

```C
	if (gj_scores_add("500 midi-chlorians", "500", NULL, NULL, NULL)) {
		// Error, score not added
	}
```

#### Getting scores

You can retrieve scores from the main game table, or from any other additional tables the developer has created (using the GameJolt Web UI). First you get the raw table data, and then you iterate on scores one by one. When requesting the data, there are also some options to filter the values you get, read the documentation if you need to use them:

```C
	char *score_data;
	struct gj_score score;

	score_data = gj_scores_fetch(NULL, NULL, NULL, NULL, NULL, false);
	if (!score_data) {
		// Error fetching data
		return;
	}
	while (score_data && *score_data) {
		score_data = gj_score_get_next(score_data, &score);
		if (!score_data) {
			// Error formatting data
			return;
		}
		beatifully_print_score(&score);
	}
```

#### Updating sessions

Sessions allow to track who is playing the game and how much time. Basically you open a session, and have to periodically ping it. If you spend 120 seconds without pinging the session, it will be automatically closed, so the recommendation is to ping each 30 seconds.

```C
	// Do this just once to open the session
	if (gj_sessions_open()) {
		// Something went wrong, session not opened
	}

	// [...]

	// Do this every 30 seconds approximately
	if (gj_sessions_ping(true)) {
		// Something went wrong, ping failed
	}
```

#### Listing users and friends

You can list friends, and get detailed data of each user. To list friends, request the raw data, and then iterate on the results, one by one:

```C
	char *data, *friend_id;

	if (!(data = gj_friends_fetch())) {
		// Failed to fetch friends data
		return;
	}
	while (data && *data) {
		data = gj_friend_get_next(data, &friend_id);
		if (!data || !friend_id) {
			// Failed to iterate on friends data
			return;
		}
		use_friend_id_for_whatever(friend_id);
	}
```

And you can get user data from *username* or *user\_id* as follows:

```C
	struct gj_user user;
	char *data;

	if (!(data = gj_users_fetch("doragasu", NULL))) {
		// Fetching user failed
		return;
	}
	while (data && *data) {
		data = gj_user_get_next(data, &user);
		if (!data) {
			// Formatting user data failed
			return;
		}
		print_user_data_with_great_fx(&user);
	}
```

#### Storing and retrieving data from the cloud

You can upload data to retrieve it later. And you can even make the cloud perform basic operations on the uploaded data. This can have many uses, such as saving detailed statistics, implement cloud save game functions, make turn based multiplayer games, etc. The data can be stored on the global game store, or on the user store. An example to store data is as follows:

```C
	if (gj_data_store_set("the_meaning_of_life", "41", false)) {
		// Record on global game store failed
	}
```

To update data and perform operations on it, you can for example:

```C
	if (!gj_data_store_update("the_meaning_of_life", GJ_OP_ADD, "1", false)) {
		// Data store update failed
	}
```

You can match keys to retrieve them using the wildcard character ('\*'):

```C
	char *data;

	data = data_store_fetch("the_meaning_*", false);
	if (!data) {
		// Error fetching data
		return;
	}
	cloud_computed_meaning_of_life_is(data);
```

#### Getting error information

Most API functions return an error either via a bool value (error if true), or a data pointer (error if NULL). Internally the functions track the error with greater detail. If you want to know what caused the error, after a function fails, call `gj_get_error()`. This will allow you to know if the error was caused because of a parameter error, a request error, a server error, etc. As the internally tracked error is updated after each GameJolt API call, you have to call this function just after the one that failed.

### Test program

The main.c file contains a test program that detects the WiFi module, associates to the AP on slot 0, connects to `https://www.example.com` using both a TCP socket and an HTTPS request, displays the synchronized date/time, sends and receives using a client UDP socket, and echoes UDP data on port 8007.

### Some more tips

As previously discussed, most MegaWiFi API calls are synchronous: this means that when you call the function, the system task will be blocked until a response arrives (or a timeout occurs). This can be inconvenient, because typically you will still want to do things while waiting for the data (move backgrounds, update sprites, etc.). There are several ways to workaround this problem, some of them discussed below.

#### Use asynchronous functions to send/receive data

Usually you can separate the communications related work in two parts: *initialization stage* and *game stage*. During the *initialization stage* you will have to do tasks such as associating to the WiFi access point, connecting to the server, updating scoreboards, etc. During the *game stage* you will usually only need to send player actions to the server and get server status updates. Usually during the *initialization stage* it can be tolerable blocking and not updating the screen for short time-defined periods. But during *game stage*, usually yow want to continuously move things, so you cannot afford blocking the supervisor task. Taking this into account, a good compromise solution could be using all the MegaWiFi blocking functions during the *initialization stage*, and using the non-blocking asynchronous `mw_send()` and `mw_recv()` functions during the *game stage*.

#### Use the VBLANK interrupt to update the game status

You can use the VBLANK interrupt as an additional task to the supervisor and user task. The idea here is that the VBLANK interrupt performs all the game logic and commands the system task (via shared memory) when to send/receive data. When the data is sent/received, the code in the VBLANK interrupt will also get the notification/data via shared memory variables. The tricky part here is separating the game logic from the communications logic, and properly synchronizing the shared memory variables. Also you should make sure not touching the VDP outside the VBLANK interrupt, or you risk the code touching the VDP to be interrupted, leading to difficult to debug bugs.

#### Use the lower level MegaWiFi API

Instead of using the "higher level" MegaWiFi API, that implements the locking mechanism and requires the user task to be properly configured, you can do your alternative implementation using only the asynchronous API calls:`mw_send()`, `mw_recv()`, `mw_cmd_send()` and `mw_cmd_recv()`. This requires manually building the command frames using the formatting defined in `mw-msg.h`, and manually polling the `mw_process()` function after any of the previous functions to get the data sent/received. Note this can be very time consuming if you are using many commands that you will have to implement, and especially for the more higher level ones, like the GameJolt Game API.

#### Use the loop module

As I wrote above, previous versions of MegaWiFi came with a `loop` module. This module is a bit more complex and requires a bit more work to set up than the tasking approach, but it can be a lot more flexible: it allows setting up as many "loop functions" and "loop timers" as you need. So you can have a loop timer blocked on sending/receiving data from the module, while other is running free and updating the game without a problem.

Although the loop module is not included in current MegaWiFi releases, you can grab it from older releases (latest one including it is version 1.4). If you do, make sure you also read the documentation from that release detailing how the loop module works.

#### Do status changes in the user task

As you have seen, you are in charge of setting up the user task to call `mw_process()`. But in addition to call `mw_process()`, you can do more work in the user task if you want. Your user task could be something like:

```C
static void user_tsk(void)
{
	while (1) {
		update_game_logic();
		mw_process();
	}
}
```

Inside `update_game_logic()` you could monitor when the frame changes, and do updates to the game logic accordingly. Note this might look like an easy approach, but actually it might be the hardest one to do properly. The reason is that if you touch the VDP inside `update_game_logic()`, if the function is interrupted in the middle of changing the VDP state, weird and hard to debug bugs will occur. I would advice against this approach, unless you do a thorough timing analysis to ensure this problem cannot occur. Also in case you were thinking on disabling interrupts at the CPU level while touching the VDP to avoid this problem, I'm afraid you can not. Changing interrupts is a privileged operation, and the user task just can't do it.

## Author

This API and documentation has been written by Jesús Alonso (doragasu).

## Contributions

Contributions are welcome. If you find a bug please open an issue, and if you have implemented a cool feature/improvement, please send a pull request.

## License

The code and documentation in this repository is provided with NO WARRANTY, under the [Mozilla Public License (MPL)](https://www.mozilla.org/en-US/MPL/).

