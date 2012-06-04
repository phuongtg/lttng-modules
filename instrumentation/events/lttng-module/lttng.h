#undef TRACE_SYSTEM
#define TRACE_SYSTEM lttng

#if !defined(_TRACE_LTTNG_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_LTTNG_H

#include <linux/tracepoint.h>

#define LTTNG_UEVENT_SIZE 256

TRACE_EVENT(lttng_metadata,

	TP_PROTO(const char *str),

	TP_ARGS(str),

	/*
	 * Not exactly a string: more a sequence of bytes (dynamic
	 * array) without the length. This is a dummy anyway: we only
	 * use this declaration to generate an event metadata entry.
	 */
	TP_STRUCT__entry(
		__string(	str,		str	)
	),

	TP_fast_assign(
		tp_strcpy(str, str)
	),

	TP_printk("")
)

TRACE_EVENT(lttng_uevent,

	TP_PROTO(char *str),

	TP_ARGS(str),

	TP_STRUCT__entry(
		__array_text(	char,	text,	LTTNG_UEVENT_SIZE	)
	),

	TP_fast_assign(
		tp_memcpy(text, str, LTTNG_UEVENT_SIZE)
	),

	TP_printk("text=%s", __entry->text)
)
/*
TRACE_EVENT(lttng_uevent_cfu,

	TP_PROTO(const char *str),

	TP_ARGS(str),

	TP_STRUCT__entry(
		__string_from_user(	str,	str	)
	),

	TP_fast_assign(
		tp_copy_string_from_user(str, str)
	),

	TP_printk()
)*/

TRACE_EVENT(lttng_uevent_cfu,
	TP_PROTO(const char * str),
	TP_ARGS(str),
	TP_STRUCT__entry(__string_from_user(str, str)),
	TP_fast_assign(tp_copy_string_from_user(str, str)),
	TP_printk()
)

TRACE_EVENT(lttng_uevent_memcpy,

	TP_PROTO(const char *str, size_t len),

	TP_ARGS(str, len),

	TP_STRUCT__entry(
		__string(	text,	str	)
	),

	TP_fast_assign(
		tp_memcpy_from_user(text, str, len)
	),

	TP_printk("text=%s", __entry->text)
)

#endif /*  _TRACE_LTTNG_H */

/* This part must be outside protection */
#include "../../../probes/define_trace.h"
