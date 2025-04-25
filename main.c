/**
 * @file main.c
 *
 * @author :Yujin Yu
 * @date   :2025.03.05
 */

#include <stdio.h>
#include "ts_global.h"
#include "ts_analyzer.h"
#include "pid_save.h"
#include "slot_filter.h"
#include "get_pat_info.h"
#include "get_pmt_info.h"
#include "get_sdt_info.h"
#include "get_eit_info.h"
#include "integrate_data.h"
#include "user.h"

void process_table_info(Slot *slot)
{
	int error_code = 0;

	if (fseek(slot->ts_file, slot->start_position, SEEK_SET) != 0)
	{
		LOG("%s:%d fseek error\n", __FILE__, __LINE__);
	}

	// step4
	init_pat_resource(slot);
	init_sdt_resource(slot);
	init_eit_resource(slot); // should be free at the end of file

	if ((error_code = section_filter(slot)) < 0)
	{
		LOG("error_code = %d\n", error_code);
	}

	// For insurance purposes, actually they had been released in their callback
	free_pat_resource();
	free_pmt_resource(); // which was init in pat_callback of get_pat_info.c
	free_sdt_resource();
	free_eit_resource(); // null, should be free at the end of file

	external_interface(slot->ts_file, slot->start_position, slot->packet_size);

	return;
}

void process_ts_file(FILE *input_fp)
{
	Slot         slot                = {0};
	int          packet_size         = 0;
	long         first_sync_position = 0;

	// step1
	packet_size = detect_ts_packet_size(input_fp, &first_sync_position);
	if (packet_size < 0)
	{
		LOG("[step1 fail]: error code : %d\n", packet_size);
		return;
	}

	DOUBLE_LINE
	slot = init_slot(input_fp, (unsigned char)packet_size, first_sync_position);
	LOG("[step1 success]: Packet length: %d bytes, First packet offset: %ld bytes\n", slot.packet_size, slot.start_position);
	SINGLE_LINE;

	process_table_info(&slot);
	
	clear_slot(&slot);
	return;
}

int main(void)
{
	FILE *input_fp = NULL;

	// const char *input_file = "C:\\Users\\Administrator\\Desktop\\training\\test_code_stream\\canal.ts";
	// const char *input_file = "C:\\Users\\Administrator\\Desktop\\training\\test_code_stream\\ukdigital.ts";
	// const char *input_file = "C:\\Users\\Administrator\\Desktop\\training\\test_code_stream\\black_border.ts";
	const char *input_file ="C:\\Users\\YYJ\\Desktop\\code\\ukdigital\\ukdigital.ts";
	

	input_fp = fopen(input_file, "rb");
	if (input_fp == NULL)
	{
		LOG("open input_file fail, error_code : %d\n", OPEN_INPUT_FILE_ERROR);
		return -1;
	}

	process_ts_file(input_fp);

	fclose(input_fp);
	return 0;
}
