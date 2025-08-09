/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "eth.h"
#include "gddr.h"
#include "init.h"
#include "noc2axi.h"

#define TENSIX_L1_SIZE (1536 * 1024)

extern uint8_t large_sram_buffer[SCRATCHPAD_SIZE] __aligned(4);

void wipe_l1()
{
	uint8_t tensix_x = 1;
	uint8_t tensix_y = 2;
	uint8_t noc_id = 0;
	uint8_t tlb_entry = 0;

	// step1 wipe all tensix L1s
	// use NOC DMA broadcast
	memset(large_sram_buffer, 0, SCRATCHPAD_SIZE);
	NOC2AXITlbSetup(noc_id, tlb_entry, tensix_x, tensix_y, 0);
	void volatile *tensix_l1 = GetTlbWindowAddr(noc_id, tlb_entry, 0);
	ArcDmaTransfer(large_sram_buffer, (void *) tensix_l1, SCRATCHPAD_SIZE);

	uint32_t chunk_size = SCRATCHPAD_SIZE;
	uint32_t offset = SCRATCHPAD_SIZE;
	while (offset < TENSIX_L1_SIZE) {
		noc_dma_write(tensix_x, tensix_y, offset, tensix_x, tensix_y, remote_addr, chunk_size);
		offset += chunk_size;
		chunk_size *= 2;
	}

	noc_dma_broadcast(tensix_x, tensix_y, 0, TENSIX_L1_SIZE);

	// use ARC broadcast TLB + ARC DMA
	memset(large_sram_buffer, 0, SCRATCHPAD_SIZE);
	for (uint32_t addr = 0; addr < TENSIX_L1_SIZE; addr+=SCRATCHPAD_SIZE) {
		NOC2AXITensixBroadcastTlbSetup(noc_id, tlb_entry, addr, kNoc2AxiOrderingStrict);
		void volatile *l1 = GetTlbWindowAddr(noc_id, tlb_entry, 0);		    
		ArcDmaTransfer(large_sram_buffer, (void *) l1, SCRATCHPAD_SIZE);
	}
	
	wipe_mrisc_l1(tensix_x, tensix_y);
	wipe_erisc_l1(tensix_x, tensix_y);
}
// TODO: need to reorder priority
SYS_INIT(wipe_l1, APPLICATION, 15);
