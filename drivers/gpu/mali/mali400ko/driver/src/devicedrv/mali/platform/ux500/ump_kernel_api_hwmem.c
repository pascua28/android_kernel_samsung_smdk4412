/*
 * Copyright (C) ST-Ericsson SA 2011
 * Author: Magnus Wendt <magnus.wendt@stericsson.com> for
 * ST-Ericsson.
 * License terms: GNU General Public License (GPL), version 2.
 */

#include "ump_kernel_types.h"
#include "mali_kernel_common.h"

#include <linux/hwmem.h>
#include <linux/err.h>


/* The UMP kernel API for hwmem has been mapped so that
 * ump_dd_handle == hwmem_alloc
 * ump_secure_id == hwmem global name
 *
 * The current implementation is limited to contiguous memory
 */

ump_secure_id ump_dd_secure_id_get(ump_dd_handle memh)
{
	int hwmem_name = hwmem_get_name((struct hwmem_alloc *) memh);

	if (unlikely(hwmem_name < 0)) {
		MALI_DEBUG_PRINT(1, ("%s: Invalid Alloc 0x%x\n",__func__, memh));
		return UMP_INVALID_SECURE_ID;
	}

	return (ump_secure_id)hwmem_name;
}



ump_dd_handle ump_dd_handle_create_from_secure_id(ump_secure_id secure_id)
{
	struct hwmem_alloc *alloc;
	enum hwmem_mem_type mem_type;
	enum hwmem_access access;

	alloc = hwmem_resolve_by_name((int) secure_id);

	if (IS_ERR(alloc)) {
		MALI_DEBUG_PRINT(1, ("%s: Invalid UMP id %d\n",__func__, secure_id));
		return UMP_DD_HANDLE_INVALID;
	}

	hwmem_get_info(alloc, NULL, &mem_type, &access);

	if (unlikely((access & (HWMEM_ACCESS_READ | HWMEM_ACCESS_WRITE | HWMEM_ACCESS_IMPORT)) !=
	                       (HWMEM_ACCESS_READ | HWMEM_ACCESS_WRITE | HWMEM_ACCESS_IMPORT))) {
		MALI_DEBUG_PRINT(1, ("%s: Access denied on UMP id %d, (access==%d)\n",
			__func__, secure_id, access));
		hwmem_release(alloc);
		return UMP_DD_HANDLE_INVALID;
	}

	if (unlikely(HWMEM_MEM_CONTIGUOUS_SYS != mem_type)) {
		MALI_DEBUG_PRINT(1, ("%s: UMP id %d is non-contiguous! (not supported)\n",
			__func__, secure_id));
		hwmem_release(alloc);
		return UMP_DD_HANDLE_INVALID;
	}

	return (ump_dd_handle)alloc;
}



unsigned long ump_dd_phys_block_count_get(ump_dd_handle memh)
{
	return 1;
}



ump_dd_status_code ump_dd_phys_blocks_get(ump_dd_handle memh,
                                          ump_dd_physical_block * blocks,
                                          unsigned long num_blocks)
{
	struct hwmem_mem_chunk hwmem_mem_chunk;
	size_t hwmem_mem_chunk_length = 1;

	int hwmem_result;
	struct hwmem_alloc *alloc = (struct hwmem_alloc *)memh;

	if (unlikely(blocks == NULL)) {
		MALI_DEBUG_PRINT(1, ("%s: blocks == NULL\n",__func__));
		return UMP_DD_INVALID;
	}

	if (unlikely(1 != num_blocks)) {
		MALI_DEBUG_PRINT(1, ("%s: num_blocks == %d (!= 1)\n",__func__, num_blocks));
		return UMP_DD_INVALID;
	}

	MALI_DEBUG_PRINT(5, ("Returning physical block information. Alloc: 0x%x\n", memh));

	/* It might not look natural to pin here, but it matches the usage by the mali kernel module */
	hwmem_result = hwmem_pin(alloc, &hwmem_mem_chunk, &hwmem_mem_chunk_length);

	if (unlikely(hwmem_result < 0)) {
		MALI_DEBUG_PRINT(1, ("%s: Pin failed. Alloc: 0x%x\n",__func__, memh));
		return UMP_DD_INVALID;
	}

	blocks[0].addr = hwmem_mem_chunk.paddr;
	blocks[0].size = hwmem_mem_chunk.size;

	hwmem_set_domain(alloc, HWMEM_ACCESS_READ | HWMEM_ACCESS_WRITE,
		HWMEM_DOMAIN_SYNC, NULL);

	return UMP_DD_SUCCESS;
}



ump_dd_status_code ump_dd_phys_block_get(ump_dd_handle memh,
                                         unsigned long index,
                                         ump_dd_physical_block * block)
{
	if (unlikely(0 != index))	{
		MALI_DEBUG_PRINT(1, ("%s: index == %d (!= 0)\n",__func__, index));
		return UMP_DD_INVALID;
	}
	return ump_dd_phys_blocks_get(memh, block, 1);
}



unsigned long ump_dd_size_get(ump_dd_handle memh)
{
	struct hwmem_alloc *alloc = (struct hwmem_alloc *)memh;
	int size;

	hwmem_get_info(alloc, &size, NULL, NULL);

	return size;
}



void ump_dd_reference_add(ump_dd_handle memh)
{
	/* This is never called from tha mali kernel driver */
}



void ump_dd_reference_release(ump_dd_handle memh)
{
	struct hwmem_alloc *alloc = (struct hwmem_alloc *)memh;

	hwmem_unpin(alloc);
	hwmem_release(alloc);

	return;
}
