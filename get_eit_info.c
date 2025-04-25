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
#include "get_eit_info.h"

EitList *eit_list = NULL;

static void clear_eit_loop_info(EitNode *temp_eit_node)
{
	temp_eit_node->event_id       = 0;
	temp_eit_node->start_time     = 0;
	temp_eit_node->duration       = 0;
	temp_eit_node->running_status = 0;
	temp_eit_node->free_CA_mode   = 0;

	temp_eit_node->short_event_descriptor_list        = NULL;
	temp_eit_node->extended_event_descriptor_list     = NULL;
	temp_eit_node->time_shifted_event_descriptor_list = NULL;

	temp_eit_node->next = NULL;
}

void free_short_event_descriptor_list(ShortEventDescriptorList *short_event_descriptor_list)
{
	ShortEventDescriptorNode *next_short_event_descriptor_node = NULL;
	while (short_event_descriptor_list != NULL)
	{
		next_short_event_descriptor_node = short_event_descriptor_list->next;
		free(short_event_descriptor_list);
		short_event_descriptor_list = next_short_event_descriptor_node;
	}
}

void free_extended_event_descriptor_list(ExtendedEventDescriptorList *extended_event_descriptor_list)
{
	ExtendedEventDescriptorNode *next_extended_event_descriptor_node = NULL;
	while (extended_event_descriptor_list != NULL)
	{
		next_extended_event_descriptor_node = extended_event_descriptor_list->next;
		free(extended_event_descriptor_list);
		extended_event_descriptor_list = next_extended_event_descriptor_node;
	}
}

void free_time_shifted_event_descriptor_list(TimeShiftedEventDescriptorList *time_shifted_event_descriptor_list)
{
	TimeShiftedEventDescriptorNode *next_time_shifted_event_descriptor_node = NULL;
	while (time_shifted_event_descriptor_list != NULL)
	{
		next_time_shifted_event_descriptor_node = time_shifted_event_descriptor_list->next;
		free(time_shifted_event_descriptor_list);
		time_shifted_event_descriptor_list = next_time_shifted_event_descriptor_node;
	}
}

static void free_list_in_eit_node(EitNode *eit_node)
{
	free_short_event_descriptor_list(eit_node->short_event_descriptor_list);
	free_extended_event_descriptor_list(eit_node->extended_event_descriptor_list);
	free_time_shifted_event_descriptor_list(eit_node->time_shifted_event_descriptor_list);

	return;
}

// Sort priority: service_id > transport_stream_id > original_network_id > start_time
static EitList *delete_nodes_in_eit_list_by_time(EitList *eit_list, const EitNode *temp_eit_node)
{
	EitNode *current   = eit_list;
	EitNode *prev      = NULL;
	EitNode *to_delete = NULL;

	const unsigned long start_time      = temp_eit_node->start_time;
	const unsigned long end_time        = start_time + temp_eit_node->duration;
	unsigned long       temp_start_time = 0;
	unsigned long       temp_end_time   = 0;

	if (temp_eit_node == NULL)
	{
		return eit_list;
	}

	while (current != NULL)
	{
		if (current->service_id > temp_eit_node->service_id)
			break;

		if ((current->service_id == temp_eit_node->service_id) &&
		    (current->transport_stream_id == temp_eit_node->transport_stream_id) &&
		    (current->original_network_id == temp_eit_node->original_network_id))
		{
			temp_start_time = current->start_time;
			temp_end_time   = current->start_time + temp_eit_node->duration;

			if (((temp_start_time >= start_time) && (temp_start_time <= end_time)) ||
			    ((temp_end_time >= start_time) && (temp_end_time <= end_time)))
			{
				to_delete = current;

				if (prev == NULL)
				{
					eit_list = current->next;
				}
				else
				{
					prev->next = current->next;
				}

				current = current->next;

				free_list_in_eit_node(to_delete);
				free(to_delete);

				continue;
			}
		}

		prev    = current;
		current = current->next;
	}

	return eit_list;
}

ShortEventDescriptorList *add_short_event_descriptor_node_to_list(ShortEventDescriptorList *short_event_descriptor_list,
                                                                  ShortEventDescriptorNode  temp_short_event_descriptor_node)
{
	ShortEventDescriptorNode *new_node = NULL;
	ShortEventDescriptorNode *current  = short_event_descriptor_list;
	while (current != NULL)
	{
		if ((current->ISO_639_language_code == temp_short_event_descriptor_node.ISO_639_language_code) &&
		    (strncmp(current->name, temp_short_event_descriptor_node.name, 36) == 0) &&
		    (strncmp(current->text, temp_short_event_descriptor_node.text, 36) == 0))
		{
			return short_event_descriptor_list;
		}
		current = current->next;
	}

	new_node = (ShortEventDescriptorNode *)malloc(sizeof(ShortEventDescriptorNode));
	if (new_node == NULL)
	{
		LOG("malloc error\n");
		return short_event_descriptor_list;
	}

	memcpy(new_node, &temp_short_event_descriptor_node, sizeof(ShortEventDescriptorNode));
	new_node->next = short_event_descriptor_list;

	return new_node;
}

ExtendedEventDescriptorList *add_extended_event_descriptor_node_list(ExtendedEventDescriptorList *extended_event_descriptor_list,
                                                                     ExtendedEventDescriptorNode  temp_extended_event_descriptor_node)
{
	ExtendedEventDescriptorNode *new_extended_event_descriptor_node = (ExtendedEventDescriptorNode *)malloc(sizeof(ExtendedEventDescriptorNode));
	if (new_extended_event_descriptor_node == NULL)
	{
		LOG("add_extended_event_descriptor_node_list malloc error\n");
		return extended_event_descriptor_list;
	}

	memcpy(new_extended_event_descriptor_node, &temp_extended_event_descriptor_node, sizeof(ExtendedEventDescriptorNode));
	new_extended_event_descriptor_node->next = extended_event_descriptor_list;

	return new_extended_event_descriptor_node;
}

TimeShiftedEventDescriptorList *add_time_shifted_event_descriptor_node_list(TimeShiftedEventDescriptorList *time_shifted_event_descriptor_list,
                                                                            TimeShiftedEventDescriptorNode  temp_time_shifted_event_descriptor_node)
{
	TimeShiftedEventDescriptorNode *new_time_shifted_event_descriptor_node = (TimeShiftedEventDescriptorNode *)malloc(sizeof(TimeShiftedEventDescriptorNode));
	if (new_time_shifted_event_descriptor_node == NULL)
	{
		LOG("add_time_shifted_event_descriptor_node_list malloc error\n");
		return time_shifted_event_descriptor_list;
	}

	memcpy(new_time_shifted_event_descriptor_node, &temp_time_shifted_event_descriptor_node, sizeof(TimeShiftedEventDescriptorNode));
	new_time_shifted_event_descriptor_node->next = time_shifted_event_descriptor_list;

	return new_time_shifted_event_descriptor_node;
}

static void parser_eit_descriptors(unsigned char *buffer, int max_read_position, EitNode *temp_eit_node)
{
	int           read_position      = 0;
	unsigned char descriptors_tag    = 0;
	unsigned char descriptors_length = 0;
	unsigned char name_length        = 0;
	unsigned char text_length        = 0;
	unsigned char item_length        = 0;
	unsigned char copy_length        = 0;

	ShortEventDescriptorNode       temp_short_event_descriptor_node;        // 0x4D
	ExtendedEventDescriptorNode    temp_extended_event_descriptor_node;     // 0x4E
	TimeShiftedEventDescriptorNode temp_time_shifted_event_descriptor_node; // 0x4F

	while (read_position < max_read_position)
	{
		descriptors_tag    = buffer[read_position + 0];
		descriptors_length = buffer[read_position + 1];
		read_position += 2;

		if (descriptors_length + read_position > max_read_position)
			break;

		switch (descriptors_tag)
		{
		case 0x4D:
			// LOG("0x4D\n");
			memset(&temp_short_event_descriptor_node, 0, sizeof(ShortEventDescriptorNode));
			temp_short_event_descriptor_node.ISO_639_language_code = (buffer[read_position + 0] << 16) |
			                                                         (buffer[read_position + 1] << 8) |
			                                                         (buffer[read_position + 2] << 0);

			name_length = buffer[read_position + 3];
			read_position += 4;

			copy_length = (name_length > descriptors_length - read_position) ? (descriptors_length - read_position) : name_length;
			memcpy(temp_short_event_descriptor_node.name, buffer + read_position, copy_length);
			temp_short_event_descriptor_node.name[copy_length] = '\0';
			read_position += copy_length;

			text_length = buffer[read_position];
			read_position += 1;

			copy_length = (text_length > descriptors_length - read_position) ? (descriptors_length - read_position) : text_length;
			memcpy(temp_short_event_descriptor_node.text, buffer + read_position, copy_length + 1);
			temp_short_event_descriptor_node.text[copy_length + 1] = '\0';
			read_position += copy_length;

			temp_eit_node->short_event_descriptor_list = add_short_event_descriptor_node_to_list(temp_eit_node->short_event_descriptor_list,
			                                                                                     temp_short_event_descriptor_node);

			break;

		case 0x4E:
			// LOG("0x4E\n");
			memset(&temp_extended_event_descriptor_node, 0, sizeof(ExtendedEventDescriptorNode));
			temp_extended_event_descriptor_node.descriptor_number      = (buffer[read_position + 0] >> 4) & 0x0F;
			temp_extended_event_descriptor_node.last_descriptor_number = (buffer[read_position + 0] >> 0) & 0x0F;
			temp_extended_event_descriptor_node.ISO_639_language_code  = (buffer[read_position + 1] << 16) |
			                                                            (buffer[read_position + 2] << 8) |
			                                                            (buffer[read_position + 3] << 0);
			item_length = buffer[read_position + 4];
			if (item_length > 0)
			{
				// parse_item_of_extended_event_descriptor(buffer + read_position + 5, item_length, &temp_extended_event_descriptor_node);
				memcpy(temp_extended_event_descriptor_node.item, buffer + read_position + 5, item_length);
			}

			read_position += 5 + item_length;

			text_length = buffer[read_position + 0];
			read_position += 1;

			copy_length = (text_length > descriptors_length - read_position) ? (descriptors_length - read_position) : text_length;
			memcpy(temp_extended_event_descriptor_node.text, buffer + read_position, copy_length);
			temp_extended_event_descriptor_node.text[copy_length] = '\0';
			read_position += copy_length;

			temp_eit_node->extended_event_descriptor_list = add_extended_event_descriptor_node_list(temp_eit_node->extended_event_descriptor_list,
			                                                                                        temp_extended_event_descriptor_node);

			break;

		case 0x4F:
			// LOG("0x4F\n");
			memset(&temp_time_shifted_event_descriptor_node, 0, sizeof(TimeShiftedEventDescriptorNode));
			temp_time_shifted_event_descriptor_node.reference_service_id = (buffer[read_position + 0] << 8) | buffer[read_position + 1];
			temp_time_shifted_event_descriptor_node.reference_event_id   = (buffer[read_position + 2] << 8) | buffer[read_position + 3];
			read_position += 4;

			temp_eit_node->time_shifted_event_descriptor_list = add_time_shifted_event_descriptor_node_list(temp_eit_node->time_shifted_event_descriptor_list,
			                                                                                                temp_time_shifted_event_descriptor_node);

			break;

		default:
			break;
		}
	}

	return;
}

static int parse_eit_info(unsigned char *buffer, int max_read_position, EitNode *temp_eit_node)
{
	int descriptors_length_count = 0;
	int read_position            = 0;

	if (max_read_position < 12)
	{
		return -1;
	}

	temp_eit_node->event_id = buffer[read_position + 0] << 8 | buffer[1];

	temp_eit_node->start_time = ((buffer[read_position + 2] << 24) << 8) |
	                            (buffer[read_position + 3] << 24) |
	                            (buffer[read_position + 4] << 16) |
	                            (buffer[read_position + 5] << 8) |
	                            (buffer[read_position + 6] << 0);

	temp_eit_node->duration = (buffer[read_position + 7] << 16) |
	                          (buffer[read_position + 8] << 8) |
	                          (buffer[read_position + 9] << 0);

	temp_eit_node->running_status = (buffer[read_position + 10] >> 5) & 0x07;
	temp_eit_node->free_CA_mode   = (buffer[read_position + 10] >> 4) & 0x01;
	descriptors_length_count      = (buffer[read_position + 10] & 0x0F) | buffer[read_position + 11];
	read_position += 12;

	if (descriptors_length_count >= 2)
	{
		parser_eit_descriptors(buffer + read_position, descriptors_length_count, temp_eit_node);
	}

	return 12 + descriptors_length_count;
}

// Sort priority: service_id > transport_stream_id > original_network_id > start_time
static int compare_nodes(const EitNode *a, const EitNode *b)
{
	if (a->service_id != b->service_id)
		return a->service_id - b->service_id;

	if (a->transport_stream_id != b->transport_stream_id)
		return a->transport_stream_id - b->transport_stream_id;

	if (a->original_network_id != b->original_network_id)
		return a->original_network_id - b->original_network_id;

	return a->start_time - b->start_time;
}

static EitList *add_eit_node_to_list(EitList *eit_list, EitNode temp_eit_node)
{
	EitNode *current  = eit_list;
	EitNode *new_node = NULL;

	new_node = malloc(sizeof(EitNode));
	if (new_node == NULL)
	{
		LOG("malloc error in add_eit_node_to_list\n");
		return eit_list;
	}

	memcpy(new_node, &temp_eit_node, sizeof(EitNode));
	new_node->short_event_descriptor_list        = temp_eit_node.short_event_descriptor_list;
	new_node->extended_event_descriptor_list     = temp_eit_node.extended_event_descriptor_list;
	new_node->time_shifted_event_descriptor_list = temp_eit_node.time_shifted_event_descriptor_list;
	new_node->next                               = NULL;

	// 插入头部或空链表
	if ((eit_list == NULL) || (compare_nodes(new_node, eit_list) < 0))
	{
		new_node->next = eit_list;
		return new_node;
	}

	// 查找插入位置
	while ((current->next != NULL) && (compare_nodes(new_node, current->next) >= 0))
	{
		current = current->next;
	}

	// 插入节点
	new_node->next = current->next;
	current->next  = new_node;

	return eit_list;
}

int eit_callback(Slot *slot, int filter_index, unsigned char *section_buffer, unsigned short pid)
{
	SectionHead section_header = {0};
	EitNode     temp_eit_node  = {0};

	int max_read_position = 0;
	int read_position     = 0;
	int copy_length       = 0;

	get_section_header(section_buffer, &section_header);
	if (section_header.section_length < EIT_HEADER_LENGTH + EIT_CRC_LENGTH)
	{
		return SDT_CALLBACK_SECTION_LENGTH_ERROR;
	}

	temp_eit_node.service_id                         = (section_buffer[3] << 8) | section_buffer[4];
	temp_eit_node.transport_stream_id                = (section_buffer[8] << 8) | section_buffer[9];
	temp_eit_node.original_network_id                = (section_buffer[10] << 8) | section_buffer[11];
	temp_eit_node.segment_last_section_number        = section_buffer[12];
	temp_eit_node.last_table_id                      = section_buffer[13];
	temp_eit_node.short_event_descriptor_list        = NULL;
	temp_eit_node.extended_event_descriptor_list     = NULL;
	temp_eit_node.time_shifted_event_descriptor_list = NULL;

	read_position     = EIT_HEADER_LENGTH;
	max_read_position = section_header.section_length - EIT_CRC_LENGTH;

	while (read_position + 12 <= max_read_position)
	{
		clear_eit_loop_info(&temp_eit_node);

		copy_length = parse_eit_info(section_buffer + read_position, max_read_position - read_position, &temp_eit_node);
		if (copy_length < 0)
			break;
		read_position += copy_length;
		eit_list = delete_nodes_in_eit_list_by_time(eit_list, &temp_eit_node);
		eit_list = add_eit_node_to_list(eit_list, temp_eit_node);
	}

	return 0;
}

void init_eit_resource(Slot *slot)
{
	unsigned char eit_filter_match[FILTER_MASK_LENGTH] = {0x47, 0x00, 0x12, 0x00, 0x4E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char eit_filter_mask[FILTER_MASK_LENGTH]  = {0xFF, 0x1F, 0xFF, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	// unsigned char eit_table_ids[2] = {0x4E, 0x6F};
	int eit_filter_index = 0;

	if (slot->ts_file == NULL)
	{
		LOG("Invalid parameters, error code : %d\n", EIT_INIT_PARAM_ERROR);
		return;
	}

	// alloc_filter thrice
	eit_filter_index = alloc_filter(slot, eit_filter_match, eit_filter_mask, EIT_CRC_CHECK, eit_callback);
	if (eit_filter_index < 0)
	{
		LOG("alloc_filter error\n");
		return;
	}

	eit_filter_match[4] = 0x50;
	eit_filter_mask[4]  = 0xF0;
	eit_filter_index    = alloc_filter(slot, eit_filter_match, eit_filter_mask, EIT_CRC_CHECK, eit_callback);
	if (eit_filter_index < 0)
	{
		LOG("alloc_filter error\n");
		return;
	}

	eit_filter_match[4] = 0x60;
	eit_filter_mask[4]  = 0xF0;
	eit_filter_index    = alloc_filter(slot, eit_filter_match, eit_filter_mask, EIT_CRC_CHECK, eit_callback);
	if (eit_filter_index < 0)
	{
		LOG("alloc_filter error\n");
		return;
	}

	return;
}

void free_eit_resource(void)
{
	// does not have status
	// and the filters of eit should be free at the end of file
}

EitList *get_eit_list(void)
{
	return eit_list;
}

void free_eit_list(EitList *eit_list)
{
	EitNode *next_eit_node = NULL;
	while (eit_list != NULL)
	{
		next_eit_node = eit_list->next;
		free_list_in_eit_node(eit_list);
		free(eit_list);
		eit_list = next_eit_node;
	}

	return;
}

void printf_eit_list(EitList *eit_list)
{
	EitNode                        *eit_node                           = eit_list;
	ShortEventDescriptorNode       *short_event_descriptor_node        = NULL;
	ExtendedEventDescriptorNode    *extended_event_descriptor_node     = NULL;
	TimeShiftedEventDescriptorNode *time_shifted_event_descriptor_node = NULL;

	if (eit_node == NULL)
	{
		LOG("EIT list is empty!\n");
		return;
	}

	LOG("========================================== EIT Information ====================================================\n");
	LOG("EIT list:\n");
	while (eit_node != NULL)
	{
		// if ((eit_node->short_event_descriptor_list == NULL) &&
		//     (eit_node->extended_event_descriptor_list == NULL) &&
		//     (eit_node->time_shifted_event_descriptor_list == NULL))
		// {
		// 	eit_node = eit_node->next;
		// 	continue;
		// }

		LOG("-----------------------------------------------------------------------------------------------------------\n");
		LOG("service_id: 0x%04X | transport_stream_id: 0x%04X | original_network_id: 0x%04X | start_time: %010lu\n",
		    eit_node->service_id,
		    eit_node->transport_stream_id,
		    eit_node->original_network_id,
		    eit_node->start_time);

		LOG("event_id: 0x%04X | duration: %8u sec | running_status: %d | free_CA_mode: %d\n",
		    eit_node->event_id,
		    eit_node->duration,
		    eit_node->running_status,
		    eit_node->free_CA_mode);
		LOG("\t---------------------------------------------------------------------------------------------------\n");

		// 打印短事件描述符 (0x4D)
		short_event_descriptor_node = eit_node->short_event_descriptor_list;
		while (short_event_descriptor_node != NULL)
		{
			LOG("\t[1] [Short Event] language: %c%c%c| Name: %s | Text: %s\n",
			    (short_event_descriptor_node->ISO_639_language_code >> 16) & 0xFF,
			    (short_event_descriptor_node->ISO_639_language_code >> 8) & 0xFF,
			    (short_event_descriptor_node->ISO_639_language_code >> 0) & 0xFF,
			    short_event_descriptor_node->name,
			    short_event_descriptor_node->text);
			short_event_descriptor_node = short_event_descriptor_node->next;
		}

		// 打印扩展事件描述符 (0x4E)
		extended_event_descriptor_node = eit_node->extended_event_descriptor_list;
		while (extended_event_descriptor_node != NULL)
		{
			LOG("\t[2] [Extended Event] language: %c%c%c | Desc Num: %d/%d | Text: %s\n",
			    (extended_event_descriptor_node->ISO_639_language_code >> 16) & 0xFF,
			    (extended_event_descriptor_node->ISO_639_language_code >> 8) & 0xFF,
			    (extended_event_descriptor_node->ISO_639_language_code >> 0) & 0xFF,
			    extended_event_descriptor_node->descriptor_number,
			    extended_event_descriptor_node->last_descriptor_number,
			    extended_event_descriptor_node->text);

			LOG("\t[2.1]item:%s\n", extended_event_descriptor_node->item);

			extended_event_descriptor_node = extended_event_descriptor_node->next;
		}

		// 打印时移事件描述符 (0x4F)
		time_shifted_event_descriptor_node = eit_node->time_shifted_event_descriptor_list;
		while (time_shifted_event_descriptor_node != NULL)
		{
			LOG("\t[3] [Time Shifted] reference_service_id: 0x%04X | reference_event_idt: 0x%04X\n",
			    time_shifted_event_descriptor_node->reference_service_id,
			    time_shifted_event_descriptor_node->reference_event_id);
			time_shifted_event_descriptor_node = time_shifted_event_descriptor_node->next;
		}

		eit_node = eit_node->next;
	}

	DOUBLE_LINE
}
