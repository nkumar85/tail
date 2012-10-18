#ifndef __TAIL_LOG_H__
#define __TAIL_LOG_H__

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define TAIL_LOG_SYSCALL	0x00000001
#define TAIL_LOG_ERROR		0x00000002
#define TAIL_LOG_WARN		0x00000003	
#define TAIL_LOG_DEBUG		0x00000004

typedef void (*tail_log_fn)(const char* message, va_list args);

extern uint32_t debug_level;

static inline void
tail_log_syscall (const char* message, va_list args)
{
	int err_no = 0;

	err_no = va_arg(args, int);	
	vprintf(message, args);
	printf("TAIL SYSCALL ERROR: %s\n", strerror(err_no));
}

static inline void
tail_log_error (const char* message, va_list args)
{
	printf("TAIL ERROR: ");
	vprintf(message, args);
}

static inline void
tail_log_warn (const char* message, va_list args)
{
	printf("TAIL WARN: ");
	vprintf(message, args);
}

static inline void
tail_log_debug (const char* message, va_list args)
{
	printf("TAIL DEBUG: ");
	vprintf(message, args);
}

static tail_log_fn tail_log_fn_vec[] = {
	[TAIL_LOG_SYSCALL-1]	= tail_log_syscall,
	[TAIL_LOG_ERROR-1]		= tail_log_error,
	[TAIL_LOG_WARN-1]		= tail_log_warn,
	[TAIL_LOG_DEBUG-1]		= tail_log_debug
};

static inline void
tail_log (uint32_t level, const char* message, ...)
{
	if (level <= debug_level) {
		va_list args;
		va_start(args, message);
		tail_log_fn_vec[level-1](message, args);
		va_end(args);
	}
}

#endif
