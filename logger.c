/*
 * logger.c
 * --------
 * Implements an asynchronous, thread-safe logging system using a linked-list
 * message queue and a dedicated background thread. Producers (client handler
 * threads) call logger_log(), which formats the message, appends it to the
 * tail of the queue, and signals the condition variable. The consumer (the
 * logger thread) wakes on the signal, drains the queue, and writes each
 * entry to stdout.
 *
 * This design ensures that logging never blocks a client handler thread on
 * slow I/O, and that log lines from concurrent threads are serialised through
 * the consumer without interleaving.
 *
 * Dependencies: logger.h, <stdio.h>, <stdlib.h>, <string.h>,
 *               <stdarg.h>, <time.h>, <pthread.h>
 */

#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

/* Maximum length of a single formatted log line (timestamp + message). */
#define LOG_LINE_MAX 2200

/*
 * LogEntry
 * --------
 * A single node in the singly-linked log queue. Each entry holds one
 * fully formatted log line (including timestamp) and a pointer to the next
 * entry so that the logger thread can drain the queue in FIFO order.
 */
typedef struct LogEntry {
    char            line[LOG_LINE_MAX + 32];  /* +32 for timestamp prefix */
    struct LogEntry *next;
} LogEntry;

/* Queue head and tail pointers. Both NULL means the queue is empty. */
static LogEntry *queue_head = NULL;
static LogEntry *queue_tail = NULL;

/* Mutex protecting the queue and the running flag. */
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Condition variable: signalled whenever a new entry is enqueued. */
static pthread_cond_t  log_cond  = PTHREAD_COND_INITIALIZER;

/* Set to 0 by logger_shutdown() to tell the background thread to exit. */
static int running = 0;

/* The background consumer thread. */
static pthread_t log_thread;

/*
 * logger_thread_fn(arg)
 * ---------------------
 * Body of the background logging thread. Waits on log_cond until entries
 * are available or running becomes 0. On each wake, drains the entire queue
 * by printing each line and freeing its memory. Exits once running == 0
 * and the queue is empty.
 */
static void *logger_thread_fn(void *arg) {
    (void)arg;

    pthread_mutex_lock(&log_mutex);
    while (1) {
        /* Wait until there is something to do. */
        while (running && queue_head == NULL) {
            pthread_cond_wait(&log_cond, &log_mutex);
        }

        /* Drain the entire queue while holding the mutex briefly. */
        LogEntry *head = queue_head;
        queue_head = NULL;
        queue_tail = NULL;
        pthread_mutex_unlock(&log_mutex);

        /* Print and free outside the mutex so producers aren't blocked. */
        while (head != NULL) {
            LogEntry *next = head->next;
            fputs(head->line, stdout);
            fflush(stdout);
            free(head);
            head = next;
        }

        pthread_mutex_lock(&log_mutex);

        /* If shutdown was requested and the queue is empty, we are done. */
        if (!running && queue_head == NULL) {
            break;
        }
    }
    pthread_mutex_unlock(&log_mutex);
    return NULL;
}

/*
 * logger_init()
 * -------------
 * Sets running = 1 and spawns the background thread. If pthread_create
 * fails, prints a warning — logging will be silently dropped until shutdown.
 */
void logger_init(void) {
    running = 1;
    if (pthread_create(&log_thread, NULL, logger_thread_fn, NULL) != 0) {
        fprintf(stderr, "[Logger] Failed to start logging thread.\n");
        running = 0;
    }
}

/*
 * logger_log(fmt, ...)
 * --------------------
 * Formats the caller's message with a "[YYYY-MM-DD HH:MM:SS]" prefix,
 * allocates a LogEntry, and appends it to the queue tail. Signals log_cond
 * so the consumer thread wakes up. Returns without blocking.
 *
 * Drops the message (prints a stderr warning) if malloc fails or the logger
 * has not been initialised / has been shut down.
 */
void logger_log(const char *fmt, ...) {
    pthread_mutex_lock(&log_mutex);
    if (!running) {
        pthread_mutex_unlock(&log_mutex);
        return;
    }
    pthread_mutex_unlock(&log_mutex);

    /*
     * Format the caller's message into a buffer that is deliberately smaller
     * than LOG_LINE_MAX by the size of the timestamp prefix (~32 bytes).
     * This lets GCC statically verify that the final snprintf into line[]
     * (which is LOG_LINE_MAX + 32) cannot overflow.
     */
    /*
     * Declare msg 32 bytes shorter than LOG_LINE_MAX so GCC can statically
     * verify that prepending the 22-char timestamp prefix into line[]
     * (LOG_LINE_MAX + 32) cannot truncate.
     */
    char msg[LOG_LINE_MAX - 32];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    /* Prepend a timestamp. Extra 32 bytes for "[YYYY-MM-DD HH:MM:SS] \n". */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char line[LOG_LINE_MAX + 32];
    snprintf(line, sizeof(line), "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec, msg);

    /* Allocate and populate the queue entry. */
    LogEntry *entry = malloc(sizeof(LogEntry));
    if (!entry) {
        fprintf(stderr, "[Logger] malloc failed, dropping log entry.\n");
        return;
    }
    strncpy(entry->line, line, LOG_LINE_MAX - 1);
    entry->line[LOG_LINE_MAX - 1] = '\0';
    entry->next = NULL;

    /* Enqueue and wake the consumer. */
    pthread_mutex_lock(&log_mutex);
    if (queue_tail == NULL) {
        queue_head = entry;
        queue_tail = entry;
    } else {
        queue_tail->next = entry;
        queue_tail = entry;
    }
    pthread_cond_signal(&log_cond);
    pthread_mutex_unlock(&log_mutex);
}

/*
 * logger_shutdown()
 * -----------------
 * Sets running = 0, broadcasts to the condition variable so the logger
 * thread wakes even if the queue is empty, then joins the thread to ensure
 * all pending entries are flushed before this function returns.
 */
void logger_shutdown(void) {
    pthread_mutex_lock(&log_mutex);
    running = 0;
    pthread_cond_broadcast(&log_cond);
    pthread_mutex_unlock(&log_mutex);

    pthread_join(log_thread, NULL);
}
