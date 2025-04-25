/*
 * @file get_pmt_info.c
 *
 * @author :Yujin Yu
 * @date   :2025.04.02
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ts_global.h"
#include "slot_filter.h"
#include "parse_tables_status.h"
#include "get_pat_info.h"
#include "get_pmt_info.h"

static PmtList         *pmt_list              = NULL;
static TableStatusList *pmt_table_status_list = NULL;

// Initialize descriptor collection
void init_descriptor_collection(DescriptorCollection *collection)
{
	collection->private_data_specifier_count = 0;
	collection->subtitling_descriptor_count  = 0;
}

// Parse descriptors from buffer
void parse_pmt_descriptors(unsigned char *buffer, int max_read_position, DescriptorCollection *collection)
{
	PrivateDataSpecifierDescriptor *print_private_data_specifiers = NULL;
	SubtitlingDescriptor           *subtitling_descriptor         = NULL;

	int           read_position          = 0;
	unsigned char tag                    = 0;
	unsigned char descriptor_info_length = 0;
	int           info_count             = 0;
	int           offset                 = 0;
	int           i                      = 0;

	while (read_position < max_read_position)
	{
		if (read_position + 2 > max_read_position) // at least tag + max_read_position
			break;

		tag                    = buffer[read_position];
		descriptor_info_length = buffer[read_position + 1];

		if (read_position + 2 + descriptor_info_length > max_read_position)
			break;

		switch (tag)
		{
		case DESCRIPTOR_PRIVATE_DATA_SPECIFIER:
			if (collection->private_data_specifier_count < 16 && descriptor_info_length >= 4)
			{
				print_private_data_specifiers       = &collection->private_data_specifiers[collection->private_data_specifier_count++];
				print_private_data_specifiers->type = DESCRIPTOR_PRIVATE_DATA_SPECIFIER;

				print_private_data_specifiers->private_data_specifier = (buffer[read_position + 2] << 24) |
				                                                        (buffer[read_position + 3] << 16) |
				                                                        (buffer[read_position + 4] << 8) |
				                                                        (buffer[read_position + 5] << 0);
			}
			break;

		case DESCRIPTOR_SUBTITLING:
			if (collection->subtitling_descriptor_count < 8 && descriptor_info_length >= 8)
			{
				subtitling_descriptor       = &collection->subtitling_descriptors[collection->subtitling_descriptor_count++];
				subtitling_descriptor->type = DESCRIPTOR_SUBTITLING;

				info_count = descriptor_info_length / 8;
				if (info_count > 8)
					info_count = 8;
				subtitling_descriptor->subtitling_info_count = info_count;
				// clang-format off
				for (i=0; i<info_count; i++)
				{ // clang-format on
					offset = read_position + 2 + i * 8;

					subtitling_descriptor->subtitling_info[i].ISO_639_language_code = (buffer[offset] << 16) | (buffer[offset + 1] << 8) | buffer[offset + 2];
					subtitling_descriptor->subtitling_info[i].subtitling_type       = buffer[offset + 3];
					subtitling_descriptor->subtitling_info[i].composition_page_id   = (buffer[offset + 4] << 8) | buffer[offset + 5];
					subtitling_descriptor->subtitling_info[i].ancillary_page_id     = (buffer[offset + 6] << 8) | buffer[offset + 7];
				}
			}
			break;

		default:
			break;
		}

		read_position += 2 + descriptor_info_length;
	}
}

void free_pmt_es_list(PmtESList *pmt_es_list)
{
	PmtESNode *next_es = NULL;

	while (pmt_es_list != NULL)
	{
		next_es = pmt_es_list->next;
		free(pmt_es_list);
		pmt_es_list = next_es;
	}

	return;
}

void free_pmt_list(PmtList *pmt_list)
{
	PmtNode *next_pmt = NULL;

	while (pmt_list != NULL)
	{
		free(pmt_list->es_info_list);

		next_pmt = pmt_list->next;
		free(pmt_list);
		pmt_list = next_pmt;
	}
	return;
}

PmtESList *add_pmt_es_node_to_list(PmtESList *pmt_es_list, PmtESNode temp_es_node)
{
	PmtESNode *new_node = (PmtESNode *)malloc(sizeof(PmtESNode));
	if (new_node == NULL)
	{
		LOG("Memory allocation error\n");
		return pmt_es_list;
	}

	memcpy(new_node, &temp_es_node, sizeof(PmtESNode));
	new_node->next = pmt_es_list;

	return new_node;
}

static PmtList *add_pmt_node_to_list(PmtList *pmt_list, PmtNode temp_pmt_node)
{
	PmtNode *new_node = (PmtNode *)malloc(sizeof(PmtNode));
	if (new_node == NULL)
	{
		LOG("Memory allocation error\n");
		return pmt_list;
	}
	memcpy(new_node, &temp_pmt_node, sizeof(PmtNode));
	new_node->next = NULL;

	if (pmt_list == NULL)
	{
		return new_node;
	}

	if (new_node->program_number < pmt_list->program_number)
	{
		new_node->next = pmt_list;
		return new_node;
	}

	PmtNode *current = pmt_list;
	while (current->next != NULL && current->next->program_number < new_node->program_number)
	{
		current = current->next;
	}

	new_node->next = current->next;
	current->next  = new_node;

	return pmt_list;
}

// Write ES node information
static int write_to_pmt_es_node(unsigned char *buffer, int max_read_position, PmtESNode *es_node)
{
	int es_info_length = 0;

	if (5 > max_read_position)
		return -1;

	es_info_length = ((buffer[3] & 0x0F) << 8) | buffer[+4];
	if (5 + es_info_length > max_read_position)
		return -1;

	es_node->stream_type    = buffer[0];
	es_node->elementary_pid = ((buffer[1] & 0x1F) << 8) | buffer[2];

	init_descriptor_collection(&es_node->descriptors);
	if (es_info_length > 0)
	{
		parse_pmt_descriptors(buffer + 5, es_info_length, &es_node->descriptors);
	}
	return 5 + es_info_length;
}

int pmt_callback(Slot *slot, int filter_index, unsigned char *section_buffer, unsigned short pid)
{
	TableStatusNode *table_status_node = NULL;
	SectionHead      section_header    = {0};
	PmtNode          temp_pmt_node     = {0};
	PmtESNode        temp_es_node      = {0};


	int       max_read_position   = 0;
	int       read_position       = 0;
	int       program_info_length = 0;
	int       write_length        = 0;

	get_section_header(section_buffer, &section_header);
	if (section_header.section_length < PMT_HEADER_LENGTH + PMT_CRC_LENGTH)
	{
		LOG("section length is too unsigned short\n");
		return PMT_CALLBACK_SECTION_LENGTH_ERROR;
	}

	table_status_node = find_table_status_node_in_list(pmt_table_status_list, pid, section_header.table_id);
	if (table_status_node == NULL)
	{
		LOG("table status node is NULL\n");
		return PMT_CALLBACK_NO_STATUS_ERROR;
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

	temp_pmt_node.program_number = ((section_buffer[3]) << 8) | section_buffer[4];
	temp_pmt_node.pcr_pid        = ((section_buffer[8] & 0x1F) << 8) | section_buffer[9];
	program_info_length          = ((section_buffer[10] & 0x0F) << 8) | section_buffer[11];

	if (program_info_length > 0)
	{
		init_descriptor_collection(&temp_pmt_node.program_descriptors);
		parse_pmt_descriptors(section_buffer + PMT_HEADER_LENGTH, program_info_length, &temp_pmt_node.program_descriptors);
	}

	read_position     = PMT_HEADER_LENGTH + program_info_length;
	max_read_position = section_header.section_length - PMT_CRC_LENGTH;
	while (read_position < max_read_position)
	{
		memset(&temp_es_node, 0, sizeof(PmtESNode));
		write_length = write_to_pmt_es_node(section_buffer + read_position, max_read_position, &temp_es_node);
		if (write_length <= 0)
			break;

		temp_pmt_node.es_info_list = add_pmt_es_node_to_list(temp_pmt_node.es_info_list, temp_es_node);
		read_position += write_length;
	}

	pmt_list = add_pmt_node_to_list(pmt_list, temp_pmt_node);

	if (is_table_status_node_complete(table_status_node) == 1)
	{
		clear_filter(slot, filter_index);
		if (is_table_status_list_complete(pmt_table_status_list) == 1)
		{
			free_pmt_resource();
			set_pmt_channel_status();
			return 1;
		}
	}

	return 0;
}

// Get PMT list
void init_pmt_resource(Slot *slot, PatList *pat_list)
{
	PatList *current_pat_node             = pat_list;
	int      pmt_filter_index_array[256]  = {0};
	int      pmt_filter_index_array_count = 0;

	unsigned char pmt_filter_match[FILTER_MASK_LENGTH] = {0x47, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char pmt_filter_mask[FILTER_MASK_LENGTH]  = {0xFF, 0x1F, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	if ((slot->ts_file == NULL) || (pat_list == NULL))
	{
		LOG("Invalid parameters, error code : %d\n", PMT_INIT_PARAM_ERROR);
		return;
	}

	while (current_pat_node != NULL)
	{

		pmt_filter_match[1] = current_pat_node->program_map_PID / 256;
		pmt_filter_match[2] = current_pat_node->program_map_PID % 256;

		pmt_filter_index_array[pmt_filter_index_array_count] = alloc_filter(slot, pmt_filter_match, pmt_filter_mask, PMT_CRC_CHECK, pmt_callback);
		if (pmt_filter_index_array[pmt_filter_index_array_count] < 0)
		{
			LOG("alloc_filter error,error code : %d\n", PMT_INIT_ALLOC_FILTER_ERROR);
			return;
		}
		pmt_filter_index_array_count++;

		if (is_table_status_node_exist(pmt_table_status_list, current_pat_node->program_map_PID, PMT_TABLE_ID) != 1)
		{
			pmt_table_status_list = add_table_status_node_to_list(pmt_table_status_list, current_pat_node->program_map_PID, PMT_TABLE_ID);
		}

		current_pat_node = current_pat_node->next;
	}

	return;
}

void free_pmt_resource(void)
{
	free_table_status_list(pmt_table_status_list);
	pmt_table_status_list = NULL;
}

PmtList *get_pmt_list(void)
{
	return pmt_list;
}

static void print_private_data_specifiers(const PrivateDataSpecifierDescriptor *specifiers, int count)
{
	int i = 0;

	if (count <= 0)
		return;

	LOG("    Private Data Specifier Descriptors (%d):\n", count);
	// clang-format off
	for (i=0; i<count; i++)
	{ // clang-format on
		LOG("      [%d] Specifier: 0x%08X\n", i, specifiers[i].private_data_specifier);
	}
}

static void print_subtitling_descriptors(SubtitlingDescriptor *descriptors, int count)
{
	SubtitlingDescriptor *subtitling_descriptor = NULL;
	SubtitlingInfo       *subtitling_info       = NULL;
	int                   i = 0, j = 0;

	if (count <= 0)
		return;

	LOG("      Subtitling Descriptors (%d):\n", count);
	// clang-format off
	for (i=0; i<count; i++)
	{
		subtitling_descriptor = &descriptors[i];
		for (j=0; j<subtitling_descriptor->subtitling_info_count; j++)
		{ // clang-format on
			subtitling_info = &subtitling_descriptor->subtitling_info[j];
			LOG("        [%d.%d] Language: %c%c%c, Type: 0x%02X, Composition PID: 0x%04X, Ancillary PID: 0x%04X\n",
			    i, j,
			    (subtitling_info->ISO_639_language_code >> 16) & 0xFF,
			    (subtitling_info->ISO_639_language_code >> 8) & 0xFF,
			    (subtitling_info->ISO_639_language_code >> 0) & 0xFF,
			    subtitling_info->subtitling_type,
			    subtitling_info->composition_page_id,
			    subtitling_info->ancillary_page_id);
		}
	}
}

void printf_pmt_list(PmtNode *pmt_list)
{
	PmtNode *pmt_node      = pmt_list;
	int      program_index = 0;

	if (pmt_node == NULL)
	{
		LOG("PMT list is empty\n");
		return;
	}

	LOG("PMT list:\n");
	while (pmt_node != NULL)
	{
		LOG("\n----------------------------------------------------------------------------------------- %2d \n", ++program_index);
		LOG("Program Number: 0x%04X (%u)\tPCR PID: 0x%04X (%d)\n",
		    pmt_node->program_number, pmt_node->program_number,
		    pmt_node->pcr_pid, pmt_node->pcr_pid);

		// Print program descriptors
		print_private_data_specifiers(pmt_node->program_descriptors.private_data_specifiers,
		                              pmt_node->program_descriptors.private_data_specifier_count);

		print_subtitling_descriptors(pmt_node->program_descriptors.subtitling_descriptors,
		                             pmt_node->program_descriptors.subtitling_descriptor_count);

		// Print elementary streams
		LOG("\n  Elementary Streams:\n");
		PmtESNode *es_info  = pmt_node->es_info_list;
		int        es_index = 0;

		while (es_info != NULL)
		{
			LOG("    [%d] Stream Type: 0x%02X, PID: 0x%04X (%d)\n",
			    es_index++, es_info->stream_type, es_info->elementary_pid, es_info->elementary_pid);

			// Print ES descriptors
			print_private_data_specifiers(es_info->descriptors.private_data_specifiers,
			                              es_info->descriptors.private_data_specifier_count);

			print_subtitling_descriptors(es_info->descriptors.subtitling_descriptors,
			                             es_info->descriptors.subtitling_descriptor_count);

			es_info = es_info->next;
		}

		pmt_node = pmt_node->next;
	}
	DOUBLE_LINE
}
