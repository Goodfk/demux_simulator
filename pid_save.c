/**
 * @file pid_extractor.c
 *
 * @brief Contains functions for copying packets with a specified PID from an input file to an output file
 *
 * @author :Yujin Yu
 * @date   :2025.03.11
 */

#include <stdio.h>
#include <stdlib.h>
#include "ts_global.h"
#include "ts_analyzer.h"
#include "pid_save.h"

int is_pid_in_array(unsigned short pid, const unsigned short *pids_array, int array_count)
{
	int i = 0;
	// clang-format off
	for (i=0; i<array_count; i++)
	{ // clang-format on
		if (pid == pids_array[i])
		{
			return 1;
		}
	}
	return 0;
}

int extract_pid_packets(FILE *input_fp, long start_Position, unsigned char packet_size,
                        FILE *fp_out, const unsigned short *pids_array, int array_count)
{
	unsigned char  pkt_buf[204] = {0};
	unsigned short pid          = 0;
	int            packet_count = 0;

	if ((input_fp == NULL) || (fp_out == NULL))
	{
		LOG("%s:%d input_fp or fp_out is NULL\n", __FILE__, __LINE__);
		return -1;
	}

	if (fseek(input_fp, start_Position, SEEK_SET) != 0)
	{
		LOG("%s:%d fseek error\n", __FILE__, __LINE__);
		perror("fseek");
		return -1; // Seek failed
	}

	while (fread(pkt_buf, 1,packet_size, input_fp) == packet_size)
	{
		if (pkt_buf[0] != 0x47)
			continue;

		pid = ((pkt_buf[1] & 0x1F) << 8) | pkt_buf[2];

		if (is_pid_in_array(pid, pids_array, array_count) != 1)
			continue;

		if (fwrite(pkt_buf, 1, packet_size, fp_out) != packet_size)
		{
			LOG("fwrite error\n");
		}

		packet_count++;
    }
	return packet_count;
}
