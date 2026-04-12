/*
 * common.h
 * --------
 * Shared constants and data structures used by both the server and client.
 * This is the single source of truth for the application's protocol and
 * configuration. All modules should include this header rather than
 * defining their own magic numbers or wire formats.
 *
 * Dependencies: <time.h>
 */

#ifndef COMMON_H
#define COMMON_H

#include <time.h>

/* Maximum number of simultaneously connected clients the server will track. */
#define MAX_CLIENTS   32

/*
 * Size of the read/write buffer used for sending and receiving raw data.
 * This bounds the maximum message length a client can send in one call.
 */
#define BUFFER_SIZE   2048

/* Maximum length of a client's display nickname, including the null terminator. */
#define MAX_NICK_LEN  32

/* Default TCP port the server listens on if none is provided via argv. */
#define DEFAULT_PORT  8080

/*
 * ChatMessage
 * -----------
 * The structured wire format for all chat messages exchanged between the
 * server and clients. The server formats outgoing messages into this shape
 * before broadcasting or sending privately; clients display the fields
 * directly to the terminal.
 *
 * Fields:
 *   sender    - Display nickname of the originating client.
 *   body      - The message content (chat text or server notification).
 *   timestamp - Unix epoch time when the server processed the message.
 */
typedef struct {
    char   sender[MAX_NICK_LEN];
    char   body[BUFFER_SIZE];
    time_t timestamp;
} ChatMessage;

#endif /* COMMON_H */
