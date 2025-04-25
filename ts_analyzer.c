/**
 * @file ts_analyzer.c
 *
 * @brief This file contains a series of functions for analyzing TS files, including detecting TS packet size, validating TS,
 *        finding the next sync byte packet, etc.
 *
 * @author :Yujin Yu
 * @date   :2025.03.05
 */

#include <stdio.h>
#include "ts_global.h"
#include "ts_analyzer.h"

/**
 * @brief Validate if the packet size matches the expected value
 *
 * @param input_fp       File pointer to the input file containing the TS packets.
 * @param start_position Starting position in the file to begin validation.
 * @param packet_size    Expected packet size to validate against.
 *
 * @return 1: successful
 *         0: failed
 */
static int validate_packet_size(FILE *input_fp, int start_position, int packet_size)
{
	int           i         = 0;
	unsigned char sync_byte = 0; // Used to store the read sync byte

	if (fseek(input_fp, start_position, SEEK_SET) != 0)
	{
		return 0;
	}

	// clang-format off
	// Traverse each packet to be validated
	for (i=0; i<VALIDATION_DEPTH; i++)
	{ // clang-format on

		// Read the sync byte
		if ((sync_byte = fgetc(input_fp)) != SYNC_BYTE)
		{
			return 0; // Read failed
		}

		// Seek to the sync byte position of the current packet
		if (fseek(input_fp, packet_size - 1, SEEK_CUR) != 0)
		{
			return 0; // Seek failed
		}
	}

	return 1; // All packets validated successfully
}

int detect_ts_packet_size(FILE *input_fp, long *first_sync_position)
{
	long      current_position = 0;
	int       read_byte        = 0;
	int       i                = 0;
	const int candidates[3]    = {TS_PACKET_SIZE, TS_DVHS_PACKET_SIZE, TS_FEC_PACKET_SIZE}; // All candidate packet sizes

	if (input_fp == NULL)
	{
		LOG("input_fp is NULL\n");
		return DETECT_TS_PACKET_SIZE_PARAM_ERROR; // Parameter error
	}

	// Set the file pointer to the beginning
	if (fseek(input_fp, 0, SEEK_SET) != 0)
	{
		LOG("fseek error\n");
		return DETECT_TS_PACKET_SIZE_FSEEK_ERROR;
	}

	while ((read_byte = fgetc(input_fp)) != EOF)
	{
		if (read_byte != SYNC_BYTE)
		{
			continue;
		}

		// clang-format off
		for (i=0; i<3; i++)
		{ // clang-format on
			if (validate_packet_size(input_fp, current_position + candidates[i], candidates[i]) == 1)
			{
				*first_sync_position = current_position;
				return candidates[i];
			}

			if (fseek(input_fp, current_position, SEEK_SET) != 0) // back to current_position
			{
				LOG("fseek back error\n");
				return DETECT_TS_PACKET_SIZE_FSEEK_BACK_ERROR;
			}
		}
		current_position++;
	}

	LOG("file end\n");
	return DETECT_TS_PACKET_SIZE_FILE_END;
}
