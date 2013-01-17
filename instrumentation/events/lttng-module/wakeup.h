#undef TRACE_SYSTEM
#define TRACE_SYSTEM wakeup

#if !defined(LTTNG_WAKEUP_H_) || defined(TRACE_HEADER_MULTI_READ)
#define LTTNG_WAKEUP_H_

#include <linux/tracepoint.h>

TRACE_EVENT(dump_stack_array,
	TP_PROTO(void *addr),
	TP_ARGS(addr),
	TP_STRUCT__entry(
		__field_hex(void *, addr)
	),
	TP_fast_assign(
		tp_assign(addr, addr)
	),
	TP_printk("%p", __entry->addr)
)

#endif /* LTTNG_WAKEUP_H_ */

/* This part must be outside protection */
#include "../../../probes/define_trace.h"
