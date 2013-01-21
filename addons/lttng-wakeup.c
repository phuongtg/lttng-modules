/*
 * addons/lttng-wakeup.c
 *
 * Record more information on wakeup
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

#include "lttng-wakeup.h"
#include "../lttng-abi.h"
#include "../instrumentation/events/lttng-module/wakeup.h"

DEFINE_TRACE(testing);
DEFINE_TRACE(dump_stack_array);

#define STACK_MAX_ENTRIES 10
#define PROC_ENTRY_NAME "dump_stack"
static struct proc_dir_entry *proc_entry;

static int fault_handler(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	printk(KERN_WARNING "%s: fault %d occured in kprobe for %s\n",
			THIS_MODULE->name, trapnr, p->symbol_name);
	return 0;
}

static int trace_wakeup_hook_entry(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	if (printk_ratelimit()) {
		printk("trace_wakeup_hook_entry\n");
	}
	return 0;
}

static int trace_wakeup_hook_ret(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	if (printk_ratelimit()) {
		printk("trace_wakeup_hook_ret\n");
	}
	return 0;
}

static void my_print_stack_trace(struct stack_trace *trace)
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

static int proc_dump_stack(char *buffer, char **buffer_location,
	      off_t offset, int buffer_length, int *eof, void *data)
{
	struct stack_trace trace;
	unsigned long entries[STACK_MAX_ENTRIES];

	memset(entries, 0, sizeof(entries));
	trace.skip = 0;
	trace.nr_entries = 0;
	trace.max_entries = STACK_MAX_ENTRIES;
	trace.entries = entries;

	save_stack_trace(&trace);
	my_print_stack_trace(&trace);
	trace_dump_stack_array(trace.entries, trace.nr_entries);
	//trace_testing(trace.nr_entries);
	return 0;
}

static struct kretprobe sched_wakeup_kprobe = {
	.kp.symbol_name = "ttwu_do_wakeup",
	.kp.fault_handler = fault_handler,
	.entry_handler = trace_wakeup_hook_entry,
	.handler = trace_wakeup_hook_ret,
	.data_size = 0,
	.maxactive = 32,
};

static int __init lttng_addons_wakeup_init(void)
{
	int ret = 0;

	//ret = register_kretprobe(&sched_wakeup_kprobe);
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

	printk("lttng_addons_wakeup loaded\n");
	return 0;

error:
	unregister_kretprobe(&sched_wakeup_kprobe);
	return ret;
}

static void __exit lttng_addons_wakeup_exit(void)
{
	//unregister_kretprobe(&sched_wakeup_kprobe);
	remove_proc_entry(PROC_ENTRY_NAME, NULL);

	printk("lttng_addons_wakeup unloaded\n");
	return;
}

module_init(lttng_addons_wakeup_init);
module_exit(lttng_addons_wakeup_exit);

MODULE_LICENSE("GPL and additional rights");
MODULE_AUTHOR("Francis Giraldeau <francis.giraldeau@gmail.com>");
MODULE_DESCRIPTION("LTTng wakeup tracer");
