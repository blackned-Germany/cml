/*
 * This file is part of GyroidOS
 * Copyright(c) 2024 Fraunhofer AISEC
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
 * @file device_id.h
 *
 * Implements a device global configuration file storing device Identity
 * information. Currently this is a device unique id (uuid) only. 
 * The file is generated by the scd during first run (provisioning).
 *
 * For the cmld it is a FATAL if the file is not present and if the file does
 * not contain the required uuid.
 */

#ifndef DEVICE_ID_H
#define DEVICE_ID_H

typedef struct device_id device_id_t;

/**
 * Create a new device_id object from a config file.
 * @param path Path to the config file. Must not be NULL
 * @return The new device_id_t object or NULL on an error.
 */
device_id_t *
device_id_new(const char *path);

void
device_id_free(device_id_t *device_id);

const char *
device_id_get_uuid(const device_id_t *device_id);

#endif /* DEVICE_ID_H */
