/*
 * probes/lttng-uevent.c
 *
 * Expose kernel tracer to user-space through /proc/lttng_uevent
 *
 * Copyright (C) 2009-2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
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

#include <linux/module.h>
#include <linux/proc_fs.h>

/*
 * Create lttng_uevent tracepoint probes.
 */

#define TP_MODULE_NOAUTOLOAD
#define LTTNG_PACKAGE_BUILD
#define CREATE_TRACE_POINTS
#define TRACE_INCLUDE_PATH ../instrumentation/events/lttng-module
#include "../instrumentation/events/lttng-module/uevent.h"

#define LTTNG_UEVENT_FILE "lttng_uevent"

/**
 * lttng_uevent_write - write user-space data into kernel trace
 * @file: file pointer
 * @user_buf: user string
 * @count: length to copy
 * @ppos: unused
 *
 * Copy count bytes into a trace event "lttng_uevent".
 *
 * Returns the number of bytes copied from the source.
 * Notice that there is no guarantee that the event is
 * actually written in a trace. This can occur in 3
 * situations:
 *
 *   * Buffer overrun
 *   * No trace session are active
 *   * The data size is greater than a sub-buffer
 */

ssize_t lttng_uevent_write(struct file *file, const char __user *ubuf,
		size_t count, loff_t *fpos)
{
	trace_lttng_uevent(ubuf, count);
	return count;
}

static const struct file_operations uev_ops = {
	.owner = THIS_MODULE,
	.write = lttng_uevent_write
};

static int __init lttng_uevent_init(void)
{
	struct proc_dir_entry *uev_file;

	uev_file = create_proc_entry(LTTNG_UEVENT_FILE, 0444, NULL);
	if (!uev_file)
		return -ENOENT;
	uev_file->proc_fops = &uev_ops;
	uev_file->mode = S_IFREG | S_IWUGO;
	uev_file->uid = 0;
	uev_file->gid = 0;
	/* register manually the probe */
	__lttng_events_init__uevent();
	return 0;
}

static void __exit lttng_uevent_exit(void)
{
	/* unregister manually the probe */
	__lttng_events_exit__uevent();
	remove_proc_entry(LTTNG_UEVENT_FILE, NULL);
}

module_init(lttng_uevent_init);
module_exit(lttng_uevent_exit);

MODULE_LICENSE("GPL and additional rights");
MODULE_AUTHOR("Francis Giraldeau <francis.giraldeau@gmail.com>");
MODULE_DESCRIPTION("Append custom events to kernel trace from user-space");
