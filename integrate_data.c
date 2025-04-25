/*
 * @file get_eit_info.c
 *
 * @author :Yujin Yu
 * @date   :2025.04.16
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ts_global.h"
#include "slot_filter.h"
#include "get_pat_info.h"
#include "get_pmt_info.h"
#include "get_sdt_info.h"
#include "get_eit_info.h"
#include "integrate_data.h"

static EventDataList *add_event_data_node_to_list(EventDataList *event_data_list, EventDataNode temp_event_data_node)
{
	EventDataNode *new_node = (EventDataNode *)malloc(sizeof(EventDataNode));
	if (new_node == NULL)
	{
		LOG("malloc failed for new_node\n");
		return event_data_list;
	}

	memcpy(new_node, &temp_event_data_node, sizeof(EventDataNode));
	new_node->next = NULL;

	if (event_data_list == NULL || new_node->start_time < event_data_list->start_time)
	{
		new_node->next = event_data_list;
		return new_node;
	}

	EventDataNode *current = event_data_list;
	while (current->next != NULL && current->next->start_time < new_node->start_time)
	{
		current = current->next;
	}

	new_node->next = current->next;
	current->next  = new_node;

	return event_data_list;
}

static ProgramInfoList *add_program_info_node_to_list(ProgramInfoList *program_info_list, ProgramInfoNode temp_program_info_node)
{
	ProgramInfoNode *new_node = (ProgramInfoNode *)malloc(sizeof(ProgramInfoNode));
	if (new_node == NULL)
	{
		LOG("new_node is null\n");
		return program_info_list;
	}

	memcpy(new_node, &temp_program_info_node, sizeof(ProgramInfoNode));
	new_node->prev = NULL;
	new_node->next = NULL;

	if (program_info_list == NULL)
	{
		return new_node;
	}

	ProgramInfoNode *current = program_info_list;
	ProgramInfoNode *prev    = NULL;

	while (current != NULL && current->program_number < new_node->program_number)
	{
		prev    = current;
		current = current->next;
	}

	if (prev == NULL)
	{
		new_node->next          = program_info_list;
		program_info_list->prev = new_node;
		return new_node;
	}
	else
	{
		prev->next     = new_node;
		new_node->prev = prev;
		new_node->next = current;
		if (current != NULL)
		{
			current->prev = new_node;
		}
		return program_info_list;
	}
}

static PmtNode *find_node_in_pmt_list_by_program_number(PmtList *pmt_list, unsigned short program_number)
{
	PmtNode *current_pmt_node = pmt_list;
	while (current_pmt_node != NULL)
	{
		if (current_pmt_node->program_number == program_number)
		{
			return current_pmt_node;
		}
		current_pmt_node = current_pmt_node->next;
	}
	return NULL;
}

static int is_target_stream_type(unsigned char stream_type)
{
	switch (stream_type)
	{
	// Video stream
	case 0x01: // MPEG-1 Video
	case 0x02: // MPEG-2 Video
	case 0x1B: // H.264  Video
	case 0x24: // H.265  Video
		return 1;

	// Audio stream
	case 0x03: // MPEG-1 Audio
	case 0x04: // MPEG-2 Audio
	case 0x0F: // AAC    Audio
	case 0x80: // PCM    Audio
	case 0x81: // AC-3   Audio
	case 0x82: // DTS    Audio
	case 0x83: // TrueHD Audio
	case 0x84: // E-AC-3 Audio
	case 0x86: // DTS-HD Master Audio
		return 1;

	// Subtitle stream
	case 0x90: // PGS  Subtitle
	case 0x92: // Text Subtitle
		return 1;

	// Metadata stream
	case 0x05: // MPEG-2 Private Sections
	case 0x06: // PES Private Data
	case 0x0D: // MPEG-4 SL
	case 0x11: // LATM AAC Audio
	case 0x1C: // MPEG-4 Video
		return 1;

		// 0x81-0x8F User private stream

	default:
		return 0;
	}
}

// by pmt and pat
static ProgramInfoList *init_program_info_list(ProgramInfoList *program_info_list, PatList *pat_list)
{
	PatNode        *pat_node               = NULL;
	ProgramInfoNode temp_program_info_node = {0};

	PmtList   *pmt_list    = NULL;
	PmtNode   *pmt_node    = NULL;
	PmtESNode *pmt_es_node = NULL;

	pmt_list = get_pmt_list();
	if (pmt_list == NULL)
	{
		LOG("pmt_list is null\n");
		return NULL;
	}
	// printf_pmt_list(pmt_list);

	pat_node = pat_list;
	while (pat_node != NULL)
	{
		pmt_node = find_node_in_pmt_list_by_program_number(pmt_list, pat_node->program_number);
		if (pmt_node != NULL)
		{
			memset(&temp_program_info_node, 0, sizeof(ProgramInfoNode));

			temp_program_info_node.program_number      = pat_node->program_number;
			temp_program_info_node.transport_stream_id = pat_node->transport_stream_id;
			temp_program_info_node.pcr_pid             = pmt_node->pcr_pid;

			pmt_es_node = pmt_node->es_info_list;
			while (pmt_es_node != NULL)
			{
				if (is_target_stream_type(pmt_es_node->stream_type) == 1)
				{
					temp_program_info_node.es_info_list = add_pmt_es_node_to_list(temp_program_info_node.es_info_list, *pmt_es_node);
				}

				pmt_es_node = pmt_es_node->next;
			}

			program_info_list = add_program_info_node_to_list(program_info_list, temp_program_info_node);
		}

		pat_node = pat_node->next;
	}

	free_pmt_list(pmt_list);

	return program_info_list;
}

static int add_sdt_info_to_program_info_list(ProgramInfoList *program_info_list)
{
	SdtList         *sdt_list                  = NULL;
	ProgramInfoNode *current_program_info_node = NULL;
	SdtNode         *current_sdt_node          = NULL;

	sdt_list = get_sdt_list();
	if (sdt_list == NULL)
	{
		LOG("sdt_list is null\n");
		return -1;
	}
	// printf_sdt_list(sdt_list);

	current_sdt_node = sdt_list;
	while (current_sdt_node != NULL)
	{
		current_program_info_node = program_info_list;
		while (current_program_info_node != NULL)
		{
			if ((current_program_info_node->program_number == current_sdt_node->service_id) &&
			    (current_program_info_node->transport_stream_id == current_sdt_node->transport_stream_id) &&
			    (current_sdt_node->table_id == 0x42))
			{
				break;
			}

			current_program_info_node = current_program_info_node->next;
		}

		if (current_program_info_node != NULL)
		{
			current_program_info_node->original_network_id = current_sdt_node->original_network_id;
			current_program_info_node->service_type        = current_sdt_node->service_descriptor_list->service_type;

			memcpy(current_program_info_node->service_provider_name,
			       current_sdt_node->service_descriptor_list->service_provider_name,
			       sizeof(current_program_info_node->service_provider_name));

			memcpy(current_program_info_node->service_name,
			       current_sdt_node->service_descriptor_list->service_name,
			       sizeof(current_program_info_node->service_name));
		}

		current_sdt_node = current_sdt_node->next;
	}

	free_sdt_list(sdt_list);

	return 0;
}

static int is_unmodified(const char *str, int size)
{
	int i = 0;

	// clang-format off
	for (i=0; i<size; i++)
	{ // clang-format on
		if (str[i] != 0)
		{
			return 0;
		}
	}
	return 1;
}

// for those services which have no sdt info
static void add_infomation_to_program_info_list(ProgramInfoList *program_info_list)
{
	ProgramInfoNode *current_program_info_node = program_info_list;

	int null_service_provider_name_count = 0;
	int null_service_name_count          = 0;

	while (current_program_info_node != NULL)
	{
		if (is_unmodified(current_program_info_node->service_provider_name, MAX_NAME_LENGTH))
		{
			null_service_provider_name_count++;
			snprintf(current_program_info_node->service_provider_name, MAX_NAME_LENGTH, "provider_name_%d", null_service_provider_name_count);
			LOG("service_provider_name:%s\n", current_program_info_node->service_provider_name);
		}
		if (is_unmodified(current_program_info_node->service_name, MAX_NAME_LENGTH))
		{
			null_service_name_count++;
			snprintf(current_program_info_node->service_name, MAX_NAME_LENGTH, "service_name_%d", null_service_name_count);
			LOG("service_name:%s\n", current_program_info_node->service_name);
		}
		current_program_info_node = current_program_info_node->next;
	}

	return;
}

static int add_eit_info_to_program_info_list(ProgramInfoList *program_info_list)
{
	EitList         *eit_list                  = NULL;
	ProgramInfoNode *current_program_info_node = NULL;
	EventDataNode    temp_event_data_node      = {0};

	EitNode                        *eit_node                           = NULL;
	ShortEventDescriptorNode       *short_event_descriptor_node        = NULL;
	ExtendedEventDescriptorNode    *extended_event_descriptor_node     = NULL;
	TimeShiftedEventDescriptorNode *time_shifted_event_descriptor_node = NULL;

	eit_list = get_eit_list();
	if (eit_list == NULL)
	{
		LOG("eit_list is null\n");
		return -1;
	}
	// printf_eit_list(eit_list);

	eit_node = eit_list;
	while (eit_node != NULL)
	{
		current_program_info_node = program_info_list;
		while (current_program_info_node != NULL)
		{
			if ((current_program_info_node->program_number == eit_node->service_id) &&
			    (current_program_info_node->transport_stream_id == eit_node->transport_stream_id) &&
			    (current_program_info_node->original_network_id == eit_node->original_network_id))
			{
				break;
			}
			current_program_info_node = current_program_info_node->next;
		}

		if (current_program_info_node != NULL)
		{
			memset(&temp_event_data_node, 0, sizeof(EventDataNode));

			temp_event_data_node.event_id   = eit_node->event_id;
			temp_event_data_node.start_time = eit_node->start_time;
			temp_event_data_node.duration   = eit_node->duration;

			short_event_descriptor_node = eit_node->short_event_descriptor_list;
			while (short_event_descriptor_node != NULL)
			{
				if (strncmp(current_program_info_node->service_name, short_event_descriptor_node->name, 3) == 0)
				{
					temp_event_data_node.short_event_descriptor_list = add_short_event_descriptor_node_to_list(
					    temp_event_data_node.short_event_descriptor_list, *short_event_descriptor_node);
				}
				short_event_descriptor_node = short_event_descriptor_node->next;
			}

			extended_event_descriptor_node = eit_node->extended_event_descriptor_list;
			while (extended_event_descriptor_node != NULL)
			{
				temp_event_data_node.extended_event_descriptor_list = add_extended_event_descriptor_node_list(
				    temp_event_data_node.extended_event_descriptor_list, *extended_event_descriptor_node);

				extended_event_descriptor_node = extended_event_descriptor_node->next;
			}

			time_shifted_event_descriptor_node = eit_node->time_shifted_event_descriptor_list;
			while (time_shifted_event_descriptor_node != NULL)
			{
				temp_event_data_node.time_shifted_event_descriptor_list = add_time_shifted_event_descriptor_node_list(
				    temp_event_data_node.time_shifted_event_descriptor_list, *time_shifted_event_descriptor_node);

				time_shifted_event_descriptor_node = time_shifted_event_descriptor_node->next;
			}

			current_program_info_node->event_data_list = add_event_data_node_to_list(current_program_info_node->event_data_list, temp_event_data_node);
		}

		eit_node = eit_node->next;
	}

	free_eit_list(eit_list);

	return 0;
}

ProgramInfoList *get_program_info_list(void)
{
	ProgramInfoList *program_info_list = NULL;
	PatList         *pat_list          = NULL;
	int              error_code        = 0;

	pat_list = get_pat_list();
	if (pat_list == NULL)
	{
		LOG("pat_list is null\n");
		return NULL;
	}
	// printf_pat_list(pat_list);

	program_info_list = init_program_info_list(program_info_list, pat_list);
	if (program_info_list == NULL)
	{
		LOG("init_program_info_list error\n");
	}
	else
	{
		error_code = add_sdt_info_to_program_info_list(program_info_list);
		if (error_code < 0)
		{
			LOG("add_sdt_info_to_program_info_list error\n");
		}
		else
		{
			add_infomation_to_program_info_list(program_info_list);
			error_code = add_eit_info_to_program_info_list(program_info_list);
			if (error_code < 0)
			{
				LOG("add_eit_info_to_program_info_list error\n");
			}
		}
	}
	free_pat_list(pat_list);
	return program_info_list;
}

static void free_event_data_list(EventDataList *event_data_list)
{
	EventDataNode *next_event_data_node = NULL;
	while (event_data_list != NULL)
	{
		free_short_event_descriptor_list(event_data_list->short_event_descriptor_list);
		free_extended_event_descriptor_list(event_data_list->extended_event_descriptor_list);
		free_time_shifted_event_descriptor_list(event_data_list->time_shifted_event_descriptor_list);

		next_event_data_node = event_data_list->next;
		free(event_data_list);
		event_data_list = next_event_data_node;
	}

	return;
}

void free_program_info_list(ProgramInfoList *program_info_list)
{
	ProgramInfoNode *next_program_info_node = NULL;
	while (program_info_list != NULL)
	{
		free_event_data_list(program_info_list->event_data_list);
		free_pmt_es_list(program_info_list->es_info_list);

		next_program_info_node = program_info_list->next;
		free(program_info_list);
		program_info_list = next_program_info_node;
	}

	return;
}

ProgramInfoNode *find_program_info_by_program_number(ProgramInfoList *program_info_list, int program_number)
{
	ProgramInfoNode *current_program_info_node = program_info_list;

	while (current_program_info_node != NULL)
	{
		if (current_program_info_node->program_number == program_number)
		{
			break;
		}
		current_program_info_node = current_program_info_node->next;
	}
	return current_program_info_node;
}

void printf_program_list(ProgramInfoList *program_info_list)
{
	ProgramInfoNode *current_program_info_node = program_info_list;

	DOUBLE_LINE
	LOG("program_number |  service_name   |     provider_name       | pcr_pid \n");
	SINGLE_LINE
	while (current_program_info_node != NULL)
	{
		LOG("      %5d    | %-15s |     %-15s     | 0x%04X \n",
		    current_program_info_node->program_number,
		    current_program_info_node->service_name,
		    current_program_info_node->service_provider_name,
		    current_program_info_node->pcr_pid);
		current_program_info_node = current_program_info_node->next;
	}
	DOUBLE_LINE
}

void printf_eit_descriptor(EventDataNode *event_data_node)
{
	ShortEventDescriptorNode       *short_event_descriptor_node        = NULL;
	ExtendedEventDescriptorNode    *extended_event_descriptor_node     = NULL;
	TimeShiftedEventDescriptorNode *time_shifted_event_descriptor_node = NULL;

	LOG("\t[EIT]start_time:%010lu | duration:%u\n",
	    event_data_node->start_time,
	    event_data_node->duration);

	// 打印短事件描述符 (0x4D)
	short_event_descriptor_node = event_data_node->short_event_descriptor_list;
	while (short_event_descriptor_node != NULL)
	{
		LOG("\t   [1] [Short Event] language : %c%c%c | Name: %s | Text: %s\n",
		    (short_event_descriptor_node->ISO_639_language_code >> 16) & 0xFF,
		    (short_event_descriptor_node->ISO_639_language_code >> 8) & 0xFF,
		    (short_event_descriptor_node->ISO_639_language_code >> 0) & 0xFF,
		    short_event_descriptor_node->name,
		    short_event_descriptor_node->text);

		short_event_descriptor_node = short_event_descriptor_node->next;
	}

	// 打印扩展事件描述符 (0x4E)
	extended_event_descriptor_node = event_data_node->extended_event_descriptor_list;
	while (extended_event_descriptor_node != NULL)
	{
		LOG("\t   [2] [Extended Event] language: %c%c%c | Desc Num: %d/%d | Text: %s\n",
		    (extended_event_descriptor_node->ISO_639_language_code >> 16) & 0xFF,
		    (extended_event_descriptor_node->ISO_639_language_code >> 8) & 0xFF,
		    (extended_event_descriptor_node->ISO_639_language_code >> 0) & 0xFF,
		    extended_event_descriptor_node->descriptor_number,
		    extended_event_descriptor_node->last_descriptor_number,
		    extended_event_descriptor_node->text);

		LOG("\t\t[2.1]item:%s\n", extended_event_descriptor_node->item);

		extended_event_descriptor_node = extended_event_descriptor_node->next;
	}

	// 打印时移事件描述符 (0x4F)
	time_shifted_event_descriptor_node = event_data_node->time_shifted_event_descriptor_list;
	while (time_shifted_event_descriptor_node != NULL)
	{
		LOG("\t   [3] [Time Shifted] reference_service_id: 0x%04X | reference_event_idt: 0x%04X\n",
		    time_shifted_event_descriptor_node->reference_service_id,
		    time_shifted_event_descriptor_node->reference_event_id);

		time_shifted_event_descriptor_node = time_shifted_event_descriptor_node->next;
	}

	return;
}

void printf_more_program_info(ProgramInfoNode *program_info_node)
{
	PmtESNode     *current_es_node         = NULL;
	EventDataNode *current_event_data_node = NULL;

	if (program_info_node == NULL)
	{
		LOG("program_info_node is null\n");
		return ;
	}

	SINGLE_LINE
	LOG("program_number:%d | service_name:%s | provider_name:%s\n",
	    program_info_node->program_number,
	    program_info_node->service_name,
	    program_info_node->service_provider_name);

	LOG("\t[PMT]pcr_pid:0x%04X\n", program_info_node->pcr_pid);
	current_es_node = program_info_node->es_info_list;
	while (current_es_node != NULL)
	{
		LOG("\t   [PMT->ES]stream_type:0x%02X | elementary_pid:0x%04X\n",
		    current_es_node->stream_type,
		    current_es_node->elementary_pid);

		current_es_node = current_es_node->next;
	}

	LOG("\t----------------------------------------------------------------------------------------\n");
	current_event_data_node = program_info_node->event_data_list;
	while (current_event_data_node != NULL)
	{
		printf_eit_descriptor(current_event_data_node);

		current_event_data_node = current_event_data_node->next;
	}

	SINGLE_LINE
	return ;
}
