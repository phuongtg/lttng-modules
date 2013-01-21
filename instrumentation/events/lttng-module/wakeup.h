#undef TRACE_SYSTEM
#define TRACE_SYSTEM wakeup

#if !defined(LTTNG_WAKEUP_H_) || defined(TRACE_HEADER_MULTI_READ)
#define LTTNG_WAKEUP_H_

#include <linux/tracepoint.h>

TRACE_EVENT(dump_stack_array,
	TP_PROTO(unsigned long *entries, int nr_entries),
	TP_ARGS(entries, nr_entries),
	TP_STRUCT__entry(
		__dynamic_array_hex(unsigned long, entries, nr_entries)
	),
	TP_fast_assign(
		tp_memcpy_dyn(entries, entries)
	),
	TP_printk("") // FIXME: display the whole array
)

TRACE_EVENT(testing,
	TP_PROTO(int num),
	TP_ARGS(num),
	TP_STRUCT__entry(
		__field_hex(int, idx)
	),
	TP_fast_assign(
		tp_assign(idx, num)
	),
	TP_printk("%p", __entry->idx)
)

#endif /* LTTNG_WAKEUP_H_ */

/* This part must be outside protection */
#include "../../../probes/define_trace.h"
