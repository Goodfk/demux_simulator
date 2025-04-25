/**
 * @file get_pat_info.c
 *
 * @author :Yujin Yu
 * @date   :2025.03.25
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ts_global.h"
#include "slot_filter.h"
#include "parse_tables_status.h"
#include "get_pat_info.h"
#include "get_pmt_info.h"

static PatList         *pat_list              = NULL;
static TableStatusList *pat_table_status_list = NULL;

static void get_pat_entry_info(unsigned char *section_buffer, PatNode *temp_pat_entry_node)
{
	temp_pat_entry_node->program_number  = (section_buffer[0] << 8) | section_buffer[1];
	temp_pat_entry_node->program_map_PID = ((section_buffer[2] & 0x1F) << 8) | section_buffer[3];
	temp_pat_entry_node->next            = NULL;
}

static PatList *add_pat_entry_node_to_list(PatList *list, PatNode temp_pat_entry_node)
{
	PatNode *current = list;

	PatNode *new_pat_node = (PatNode *)malloc(sizeof(PatNode));
	if (new_pat_node == NULL)
	{
		LOG("malloc error\n");
		return list;
	}

	memcpy(new_pat_node, &temp_pat_entry_node, sizeof(PatNode));
	new_pat_node->next = NULL;

	if (current == NULL)
		return new_pat_node;

	while (current->next != NULL)
	{
		current = current->next;
	}

	current->next = new_pat_node;

	return list;
}

void free_pat_list(PatList *pat_list)
{
	PatNode *current_node = pat_list;
	PatNode *next_node    = NULL;

	while (current_node != NULL)
	{
		next_node = current_node->next;
		free(current_node);
		current_node = next_node;
	}
}

int pat_callback(Slot *slot, int filter_index, unsigned char *section_buffer, unsigned short pid)
{
	TableStatusNode *table_status_node   = NULL;
	PatNode          temp_pat_entry_node = {0};
	SectionHead      section_header      = {0};

	int max_read_position = 0;
	int read_position     = 0;

	get_section_header(section_buffer, &section_header);
	if (section_header.section_length < PAT_HEADER_LENGTH + PAT_CRC_LENGTH)
	{
		return PAT_CALLBACK_SECTION_LENGTH_ERROR;
	}

	table_status_node = find_table_status_node_in_list(pat_table_status_list, pid, section_header.table_id);
	if (table_status_node == NULL)
	{
		LOG("PAT table status node is NULL\n");
		return PAT_CALLBACK_NO_STATUS_ERROR;
	}

	if (is_table_status_node_complete(table_status_node) == 1)
	{
		return 0;
	}

	if (is_version_number_changed(table_status_node, section_header.version_number) == 1)
	{
		table_status_node->version_number      = section_header.version_number;
		table_status_node->last_section_number = section_header.last_section_number;
		memset(table_status_node->mask, 0, sizeof(table_status_node->mask));
	}

	if (is_section_repeat(table_status_node, section_header.section_number) == 1)
	{
		return 0;
	}

	set_mask_by_section_number(table_status_node, section_header.section_number);

	temp_pat_entry_node.transport_stream_id = (section_buffer[3] << 8) | section_buffer[4];

	// LOG("pat_callback:transport_stream_id:%x (%d)\n", temp_pat_entry_node.transport_stream_id, temp_pat_entry_node.transport_stream_id);

	read_position     = PAT_HEADER_LENGTH;
	max_read_position = section_header.section_length - PAT_CRC_LENGTH;

	while (read_position < max_read_position)
	{
		get_pat_entry_info(section_buffer + read_position, &temp_pat_entry_node);
		read_position += 4;

		if (temp_pat_entry_node.program_number == 0x0000)
		{
			continue;
		}

		pat_list = add_pat_entry_node_to_list(pat_list, temp_pat_entry_node);
	}

	if (is_table_status_node_complete(table_status_node) == 1)
	{
		clear_filter(slot, filter_index);
		if (is_table_status_list_complete(pat_table_status_list) == 1)
		{
			free_pat_resource();
			if (pat_list == NULL)
			{
				return PAT_CALLBACK_PAT_LIST_NULL_ERROR;
			}

			init_pmt_resource(slot, pat_list);
			return 1;
		}
	}

	return 0;
}

void init_pat_resource(Slot *slot)
{
	int pat_filter_index = -1;

	unsigned char pat_filter_match[FILTER_MASK_LENGTH] = {0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char pat_filter_mask[FILTER_MASK_LENGTH]  = {0xFF, 0x1F, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	if (slot->ts_file == NULL)
	{
		LOG("file pointer is NULL,error code : %d\n", PAT_INIT_PARAM_ERROR);
		return;
	}

	pat_filter_index = alloc_filter(slot, pat_filter_match, pat_filter_mask, PAT_CRC_CHECK, pat_callback);
	if (pat_filter_index < 0)
	{
		LOG("alloc_filter error,error code : %d\n", PAT_INIT_ALLOC_FILTER_ERROR);
		return;
	}

	if (is_table_status_node_exist(pat_table_status_list, PAT_PID, PAT_TABLE_ID) != 1)
	{
		pat_table_status_list = add_table_status_node_to_list(pat_table_status_list, PAT_PID, PAT_TABLE_ID);
	}

	return;
}

void free_pat_resource(void)
{
	free_table_status_list(pat_table_status_list);
	pat_table_status_list = NULL;
}

PatList *get_pat_list(void)
{
	return pat_list;
}

void printf_pat_list(PatList *pat_list)
{
	DOUBLE_LINE
	PatNode *current_node = pat_list;
	if (current_node == NULL)
	{
		printf("PAT list is NULL\n");
		return;
	}

	LOG("PAT list:\n");
	while (current_node != NULL)
	{
		printf("Program Number: 0x%04X (%d), Program Map PID: 0x%04X (%d), transport_stream_id: 0x%04X(%d)\n",
		       current_node->program_number, current_node->program_number,
		       current_node->program_map_PID, current_node->program_map_PID,
		       current_node->transport_stream_id, current_node->transport_stream_id);

		current_node = current_node->next;
	}
	DOUBLE_LINE
}
