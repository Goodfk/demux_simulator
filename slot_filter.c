/**
 * @file slot_filter.c
 *
 * @author :Yujin Yu
 * @date   :2025.04.14
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ts_global.h"
#include "slot_filter.h"

static unsigned int crc32_table[256];
static int          crc_table_generated = 0;

// Generate CRC32 lookup table
static void generate_crc32_table(void)
{
	unsigned int polynomial = 0x04C11DB7;
	unsigned int crc        = 0;
	int          i = 0, j = 0;

	// clang-format off
	for (i=0; i<256; i++)
	{
		crc = i << 24;
		for (j=0; j<8; j++)
		{ // clang-format on
			crc = (crc & 0x80000000) ? (crc << 1) ^ polynomial : crc << 1;
		}
		crc32_table[i] = crc;
	}
}

// Calculate CRC32 value
static unsigned int calculate_crc32(const unsigned char *data, int length)
{
	unsigned int  crc = 0xFFFFFFFF;
	unsigned char pos = 0;
	int           i   = 0;

	if (0 == crc_table_generated)
	{
		generate_crc32_table();
		crc_table_generated = 1;
	}

	// clang-format off
	for (i=0; i<length; i++)
	{ // clang-format on
		pos = (unsigned char)((crc >> 24) ^ data[i]);
		crc = (crc << 8) ^ crc32_table[pos];
	}
	return crc; // Note: No final XOR
}

// CRC verification function
static int crc_check(const unsigned char *data, int length)
{
	unsigned int computed_crc = 0;
	unsigned int stored_crc   = 0;
	if (length < 4)
		return 0;

	/* Calculate CRC for first length-4 bytes */
	computed_crc = calculate_crc32(data, length - 4);

	/* Extract stored CRC value (big-endian) */
	stored_crc = ((unsigned int)data[length - 4] << 24) |
	             ((unsigned int)data[length - 3] << 16) |
	             ((unsigned int)data[length - 2] << 8) |
	             ((unsigned int)data[length - 1] << 0);

	if (computed_crc == stored_crc)
		return 1;

	return 0;
}

Slot init_slot(FILE *ts_file, unsigned char packet_size, unsigned int start_position)
{
	Slot slot = {0};

	slot.ts_file        = ts_file;
	slot.packet_size    = packet_size;
	slot.start_position = start_position;

	return slot;
}

void clear_slot(Slot *slot)
{
	memset(slot, 0, sizeof(Slot));
}

int alloc_filter(Slot *slot, unsigned char *filter_match, unsigned char *filter_mask, int is_crc_check, parse_callback section_callback)
{
	int i = 0;

	// clang-format off
	for (i=0; i<MAX_FILTER_COUNT; i++)
	{ // clang-format on
		if (slot->filter_array[i].is_used == 0)
		{
			memset(&slot->filter_array[i], 0, sizeof(Filter));
			memcpy(slot->filter_array[i].filter_match, filter_match, FILTER_MASK_LENGTH);
			memcpy(slot->filter_array[i].filter_mask, filter_mask, FILTER_MASK_LENGTH);
			slot->filter_array[i].is_used          = 1;
			slot->filter_array[i].is_CRC_check     = is_crc_check;
			slot->filter_array[i].section_callback = section_callback;
			return i;
		}
	}

	LOG("No available filter slots\n");
	return -1;
}

void clear_filter(Slot *slot, int index)
{
	memset(&slot->filter_array[index], 0, sizeof(Filter));
}

static void get_packet_header(unsigned char *buffer, TSPacketHead *packet_header)
{
	packet_header->sync_byte                    = buffer[0];
	packet_header->transport_error_indicator    = buffer[1] >> 7;
	packet_header->payload_unit_start_indicator = (buffer[1] >> 6) & 0x01;
	packet_header->transport_priority           = (buffer[1] >> 5) & 0x01;
	packet_header->PID                          = ((buffer[1] & 0x1F) << 8) | buffer[2];
	packet_header->transport_scrambling_control = (buffer[3] >> 6) & 0x03;
	packet_header->adaptation_field_control     = (buffer[3] >> 4) & 0x03;
	packet_header->continuity_counter           = buffer[3] & 0x0F;
}

void get_section_header(unsigned char *buffer, SectionHead *section_header)
{
	section_header->table_id            = buffer[0];
	section_header->section_length      = (((buffer[1] & 0x0F) << 8) | buffer[2]) + 3;
	section_header->version_number      = (buffer[5] >> 2) & 0x1F;
	section_header->section_number      = buffer[6];
	section_header->last_section_number = buffer[7];
}

/**
 * @brief Get payload start position
 *
 * @param packet_header Pointer to TSPacketHead structure containing TS packet header info
 * @param packet_buffer  buffer containing TS packet data
 *
 * @return Payload start position
 */
static int get_payload_start_position(TSPacketHead *packet_header, unsigned char *packet_buffer)
{
	int section_start_position = 0;

	switch (packet_header->adaptation_field_control)
	{
	case 0:
		return 0;
	case 1:
		section_start_position = 4;
		break;
	case 2:
		return 0;
	case 3:
		section_start_position = 4 + 1 + packet_buffer[4];
		break;
	default:
		break;
	}

	if (packet_header->payload_unit_start_indicator == 1)
	{
		section_start_position += 1 + packet_buffer[section_start_position];
	}

	return section_start_position;
}

int compare_packet_header(unsigned char *packet_buffer, unsigned char *filter_match, unsigned char *filter_mask)
{
	const int packet_header_length = 4;
	int       i                    = 0;

	// clang-format off
	for (i=0; i<packet_header_length; i++)
	{ // clang-format on
		if ((packet_buffer[i] & filter_mask[i]) != (filter_match[i] & filter_mask[i]))
		{
			return 0;
		}
	}
	return 1;
}

int compare_section_header(unsigned char *packet_buffer, unsigned char *filter_match, unsigned char *filter_mask)
{
	const int packet_header_length = 4;
	int       i                    = 0;

	// clang-format off
	for (i=0; i<FILTER_MASK_LENGTH; i++)
	{ // clang-format on
		if ((packet_buffer[i] & filter_mask[i + packet_header_length]) != (filter_match[i + packet_header_length] & filter_mask[i + packet_header_length]))
		{
			return 0;
		}
	}
	return 1;
}

int calculate_copy_length(int payload_start_position, int packet_size, int section_length, int payload_length_count)
{
	int copy_length = packet_size - payload_start_position;

	if (section_length < copy_length + payload_length_count)
	{
		copy_length = section_length - payload_length_count;
	}
	else if (payload_length_count + copy_length > MAX_SECTION_LENGTH)
	{
		copy_length = MAX_SECTION_LENGTH - payload_length_count;
	}

	return copy_length;
}

void write_to_section_buffer(Filter *filter, unsigned char *packet_buffer, int payload_start_position, int copy_length)
{
	memcpy(filter->section_buffer + filter->payload_length_count, packet_buffer + payload_start_position, copy_length);
}

// int is_slot_no_used(Slot *slot)
// {
// 	int i = 0;

// 	// clang-format off
// 	for (i=0; i<MAX_FILTER_COUNT; i++)
// 	{ // clang-format on
// 		if (slot->filter_array[i].is_used == 1)
// 			return 0;
// 	}
// 	return 1;
// }

int section_filter(Slot *slot)
{
	TSPacketHead  packet_header      = {0};
	SectionHead   section_header     = {0};
	unsigned char packet_buffer[204] = {0};

	int payload_start_position = 0;
	int copy_length            = 0;
	int index                  = 0;
	int ret                    = 0;

	if (slot->ts_file == NULL)
	{
		return FILTER_PARAM_ERROR;
	}

	while (fread(packet_buffer, 1, slot->packet_size, slot->ts_file) == slot->packet_size)
	{
		// clang-format off
		for (index=0; index<MAX_FILTER_COUNT; index++)
		{ // clang-format on
			if (slot->filter_array[index].is_used == 0)
			{
				continue;
			}

			if (compare_packet_header(packet_buffer, slot->filter_array[index].filter_match, slot->filter_array[index].filter_mask) == 0)
				continue;

			get_packet_header(packet_buffer, &packet_header);

			payload_start_position = get_payload_start_position(&packet_header, packet_buffer);
			if ((payload_start_position < 4) || (payload_start_position > slot->packet_size - 1))
				continue;

			if (packet_header.payload_unit_start_indicator == 1)
			{
				if (compare_section_header(packet_buffer + payload_start_position, slot->filter_array[index].filter_match, slot->filter_array[index].filter_mask) == 0)
					continue;

				get_section_header(packet_buffer + payload_start_position, &section_header);

				slot->filter_array[index].is_write_flag        = 1;
				slot->filter_array[index].section_length       = section_header.section_length;
				slot->filter_array[index].payload_length_count = 0;
				memset(slot->filter_array[index].section_buffer, 0, MAX_SECTION_LENGTH);
			}

			// if(packet_header.PID==0x12)
			// {
			// 	if (section_header.table_id == 0x4e || (section_header.table_id >= 0x50 && section_header.table_id <= 0x5f))
			// 	{
			// 		LOG("eit Table_id is 0x%02X\n", section_header.table_id);
			// 	}
			// }

			if (slot->filter_array[index].is_write_flag == 0)
				continue;

			copy_length = calculate_copy_length(payload_start_position, slot->packet_size,
			                                    slot->filter_array[index].section_length, slot->filter_array[index].payload_length_count);

			write_to_section_buffer(&slot->filter_array[index], packet_buffer, payload_start_position, copy_length);
			slot->filter_array[index].payload_length_count += copy_length;

			if (slot->filter_array[index].payload_length_count < section_header.section_length)
				continue;

			if (slot->filter_array[index].is_CRC_check == 1)
			{
				if (crc_check(slot->filter_array[index].section_buffer, slot->filter_array[index].payload_length_count) != 1)
				{
					memset(slot->filter_array[index].section_buffer, 0, MAX_SECTION_LENGTH);
					slot->filter_array[index].is_write_flag        = 0;
					slot->filter_array[index].payload_length_count = 0;
					continue;
				}
			}

			if ((ret = slot->filter_array[index].section_callback(slot, index,slot->filter_array[index].section_buffer, packet_header.PID)) < 0)
			{
				LOG("error code : %d\n",ret);
			}
			memset(slot->filter_array[index].section_buffer, 0, MAX_SECTION_LENGTH);
			slot->filter_array[index].is_write_flag        = 0;
			slot->filter_array[index].payload_length_count = 0;
		}
	}

	if (fseek(slot->ts_file, slot->start_position, SEEK_SET) != 0)
	{
		LOG("fseek error\n");
	}

	LOG("file end\n");
	return 0;
}
