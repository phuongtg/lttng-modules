/*
 * addons/lttng-backtrace.c
 *
 * Record backtrace
 *
 * Copyright (C) 2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; only
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/stacktrace.h>

#include "lttng-backtrace.h"
#include "../lttng-abi.h"
#include "../instrumentation/events/lttng-module/backtrace.h"

DEFINE_TRACE(backtrace_array);

//#define FUNC_NAME "ttwu_do_wakeup"
#define FUNC_NAME "blk_update_request"
//#define FUNC_NAME "trace_block_rq_complete"
#define STACK_MAX_ENTRIES 10
#define PROC_ENTRY_NAME "backtrace"
static struct proc_dir_entry *proc_entry;

static void print_stack_trace_custom(struct stack_trace *trace)
{
	int i;
	char str[KSYM_SYMBOL_LEN];

	if (WARN_ON(!trace->entries))
		return;

	printk("trace.nr_entries=%d\n", trace->nr_entries);
	for (i = 0; i < trace->nr_entries; i++) {
		unsigned long addr = trace->entries[i];
		if (addr == ULONG_MAX)
			break;
		sprint_symbol(str, trace->entries[i]);
		printk("%p %s\n", (void *) trace->entries[i], str);
	}
}


static void record_stack_trace(struct pt_regs *regs, int verbose)
{
	struct stack_trace trace;
	unsigned long entries[STACK_MAX_ENTRIES];

	memset(entries, 0, sizeof(entries));
	trace.skip = 0;
	trace.nr_entries = 0;
	trace.max_entries = STACK_MAX_ENTRIES;
	trace.entries = entries;

	save_stack_trace_regs(regs, &trace);
	trace_backtrace_array(trace.entries, trace.nr_entries);
	if (verbose && printk_ratelimit()) {
		printk("backtrace\n");
		print_stack_trace_custom(&trace);
	}
}

static int fault_handler(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk(KERN_WARNING "%s: fault %d occured in kprobe for %s\n",
			THIS_MODULE->name, trapnr, p->symbol_name);
	return 0;
}

static int trace_stack_hook_entry(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	record_stack_trace(regs, false);
	return 0;
}

static int proc_dump_stack(char *buffer, char **buffer_location,
	      off_t offset, int buffer_length, int *eof, void *data)
{
	record_stack_trace(NULL, true);
	return 0;
}

static struct kretprobe backtrace_kprobe = {
	.kp.symbol_name = FUNC_NAME,
	.kp.fault_handler = fault_handler,
	.entry_handler = trace_stack_hook_entry,
	.handler = NULL,
	.data_size = 0,
	.maxactive = 32,
};

static int __init lttng_addons_backtrace_init(void)
{
	int ret = 0;

	ret = register_kretprobe(&backtrace_kprobe);
	if (ret < 0) {
		printk(KERN_INFO "Error loading kretprobe %d\n", ret);
		goto error;
	}

	proc_entry = create_proc_entry(PROC_ENTRY_NAME, 0644, NULL);
	if (proc_entry == NULL) {
		remove_proc_entry(PROC_ENTRY_NAME, NULL);
		ret = -ENOMEM;
		goto error;
	}
	proc_entry->read_proc = proc_dump_stack;
	proc_entry->mode = S_IFREG | S_IRUGO;
	proc_entry->uid = 0;
	proc_entry->gid = 0;

	printk("lttng_addons_backtrace loaded\n");
	return 0;

error:
	unregister_kretprobe(&backtrace_kprobe);
	return ret;
}

static void __exit lttng_addons_backtrace_exit(void)
{
	remove_proc_entry(PROC_ENTRY_NAME, NULL);
	unregister_kretprobe(&backtrace_kprobe);
	printk("lttng_addons_backtrace unloaded\n");
	return;
}

module_init(lttng_addons_backtrace_init);
module_exit(lttng_addons_backtrace_exit);

MODULE_LICENSE("GPL and additional rights");
MODULE_AUTHOR("Francis Giraldeau <francis.giraldeau@gmail.com>");
MODULE_DESCRIPTION("LTTng backtracetrace");
