/*
 * logger.h
 * --------
 * Public interface for the asynchronous server logger. Log messages are
 * enqueued by any thread via logger_log() and drained on a dedicated
 * background thread, so the calling thread never blocks on I/O.
 *
 * Lifecycle:
 *   1. Call logger_init() once at server startup.
 *   2. Call logger_log() from any thread at any time.
 *   3. Call logger_shutdown() once during server teardown to flush and stop.
 *
 * Dependencies: <stdarg.h> (variadic args for logger_log)
 */

#ifndef LOGGER_H
#define LOGGER_H

/*
 * logger_init()
 * -------------
 * Initialises the log queue and spawns the background logging thread.
 * Must be called exactly once before any logger_log() calls.
 * Prints an error and returns without starting the thread on failure.
 */
void logger_init(void);

/*
 * logger_log(fmt, ...)
 * --------------------
 * Formats a message using printf-style fmt and variadic arguments, prepends
 * a timestamp, and enqueues it for the background thread to write to stdout.
 * This function is non-blocking and thread-safe — safe to call from any
 * client handler thread simultaneously.
 */
void logger_log(const char *fmt, ...);

/*
 * logger_shutdown()
 * -----------------
 * Signals the logging thread to stop, waits for it to drain any remaining
 * queued entries, then joins the thread. Should be called once during server
 * shutdown after all client threads have exited.
 */
void logger_shutdown(void);

#endif /* LOGGER_H */
