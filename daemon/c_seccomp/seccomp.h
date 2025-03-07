/*
 * This file is part of GyroidOS
 * Copyright(c) 2013 - 2024 Fraunhofer AISEC
 * Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 (GPL 2), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GPL 2 license for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, see <http://www.gnu.org/licenses/>
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Fraunhofer AISEC <gyroidos@aisec.fraunhofer.de>
 */

/**
 * @file c_seccomp/seccomp.h
 *
 * Internal seccomp header file to define common interal structures
 * of the c_seccomp module.
 * This module provides syscall emulation as part of TSF.CML.Isolation
 */

#ifndef SECCOMP_H
#define SECCOMP_H

#include <common/event.h>
#include <common/pidfd.h>
#include <linux/seccomp.h>

typedef struct c_seccomp {
	compartment_t *compartment;
	struct seccomp_notif_sizes *notif_sizes;
	int notify_fd;
	event_io_t *event;
	unsigned int enabled_features;
	container_t *container;
	list_t *module_list; /* names of modules loaded by this compartment */
} c_seccomp_t;

bool
c_seccomp_capable(pid_t pid, uint64_t cap);

void *
c_seccomp_fetch_vm_new(c_seccomp_t *seccomp, int pid, void *rbuf, uint64_t size);

int
c_seccomp_send_vm(c_seccomp_t *seccomp, int pid, void *lbuf, void *rbuf, uint64_t size);

int
c_seccomp_emulate_mknodat(c_seccomp_t *seccomp, struct seccomp_notif *req,
			  struct seccomp_notif_resp *resp);

list_t *
c_seccomp_get_module_dependencies_new(const char *module_name);

int
c_seccomp_emulate_finit_module(c_seccomp_t *seccomp, struct seccomp_notif *req,
			       struct seccomp_notif_resp *resp);

int
c_seccomp_emulate_adjtime(c_seccomp_t *seccomp, struct seccomp_notif *req,
			  struct seccomp_notif_resp *resp);

int
c_seccomp_emulate_adjtimex(c_seccomp_t *seccomp, struct seccomp_notif *req,
			   struct seccomp_notif_resp *resp);

int
c_seccomp_emulate_settime(c_seccomp_t *seccomp, struct seccomp_notif *req,
			  struct seccomp_notif_resp *resp);

struct sock_filter *
c_seccomp_ioctl_get_filter(c_seccomp_t *seccomp, int *size);

int
c_seccomp_emulate_ioctl(c_seccomp_t *seccomp, struct seccomp_notif *req,
			struct seccomp_notif_resp *resp);

int
c_seccomp_emulate_sysinfo(c_seccomp_t *seccomp, struct seccomp_notif *req,
			  struct seccomp_notif_resp *resp);

int
c_seccomp_emulate_mount(c_seccomp_t *seccomp, struct seccomp_notif *req,
			struct seccomp_notif_resp *resp);

#endif /* SECCOMP_H */
