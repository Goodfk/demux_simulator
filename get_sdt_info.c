#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ts_global.h"
#include "slot_filter.h"
#include "parse_tables_status.h"
#include "get_sdt_info.h"

SdtList                *sdt_list              = NULL;
static TableStatusList *sdt_table_status_list = NULL;

void clear_sdt_list_by_table_id(SdtList *sdt_list, unsigned char table_id)
{
	SdtNode *current_node = sdt_list;
	SdtNode *prev_node    = NULL;

	while (current_node != NULL)
	{
		if (current_node->table_id == table_id)
		{
			prev_node->next = current_node->next;
			free(current_node);
			current_node = prev_node->next;
		}
		else
		{
			current_node = current_node->next;
		}
	}

	return;
}

void prase_sdt_info(unsigned char *section_buffer, SdtNode *temp_sdt_node, unsigned char table_id,unsigned short transport_stream_id, unsigned short temp_original_network_id)
{
	temp_sdt_node->table_id                   = table_id;
	temp_sdt_node->transport_stream_id        = transport_stream_id;
	temp_sdt_node->original_network_id        = temp_original_network_id;
	temp_sdt_node->service_id                 = (section_buffer[0] << 8) | section_buffer[1];
	temp_sdt_node->EIT_schedule_flag          = (section_buffer[2] >> 1) & 0x01;
	temp_sdt_node->EIT_present_following_flag = (section_buffer[2] >> 0) & 0x01;
	temp_sdt_node->running_status             = (section_buffer[3] >> 5) & 0x07;
	temp_sdt_node->free_CA_mode               = (section_buffer[3] >> 4) & 0x01;
	temp_sdt_node->service_descriptor_list    = NULL;
	temp_sdt_node->next                       = NULL;
}
ServiceDescriptorList *add_service_descriptor_node_to_list(ServiceDescriptorList *service_descriptor_list, ServiceDescriptorNode temp_service_descriptor_node)
{
	// ServiceDescriptorList *current_node                = service_descriptor_list;
	ServiceDescriptorList *new_service_descriptor_node = (ServiceDescriptorList *)malloc(sizeof(ServiceDescriptorList));
	if (new_service_descriptor_node == NULL)
	{
		LOG("malloc error\n");
		return service_descriptor_list;
	}

	memcpy(new_service_descriptor_node, &temp_service_descriptor_node, sizeof(ServiceDescriptorNode));
	new_service_descriptor_node->next = service_descriptor_list;
	return new_service_descriptor_node;
}
ServiceDescriptorNode *parse_sdt_descriptor_info(unsigned char *buffer, int max_read_position)
{
	ServiceDescriptorNode *service_descriptor_list = NULL;
	ServiceDescriptorNode  service_descriptor_node = {0};

	int read_position                = 0;
	int descriptors_length           = 0;
	int service_provider_name_length = 0;
	int service_name_length          = 0;

	while (read_position + 5 <= max_read_position)
	{
		memset(&service_descriptor_node, 0, sizeof(ServiceDescriptorNode));
		descriptors_length                     = buffer[read_position+1];
		service_descriptor_node.service_type   = buffer[read_position+2];
		service_provider_name_length           = buffer[read_position+3];
		read_position += 4;

		if (read_position + descriptors_length - 4 > max_read_position)
			break;

		if ((service_provider_name_length > 0) && (service_provider_name_length <= MAX_NAME_LENGTH))
		{
			memcpy(service_descriptor_node.service_provider_name, buffer + read_position, service_provider_name_length);
			service_descriptor_node.service_provider_name[service_provider_name_length] = 0;
			read_position += service_provider_name_length;
		}
		service_name_length = buffer[read_position++];

		if ((service_name_length > 0) && (service_provider_name_length <= MAX_NAME_LENGTH))
		{
			memcpy(service_descriptor_node.service_name, buffer + read_position, service_name_length);
			service_descriptor_node.service_name[service_name_length] = 0;
			read_position += service_name_length;
		}

		service_descriptor_list = add_service_descriptor_node_to_list(service_descriptor_list, service_descriptor_node);
	}
	return service_descriptor_list;
}

SdtList *add_sdt_node_to_list(SdtList *sdt_list, SdtNode temp_sdt_node)
{
	SdtNode *current  = sdt_list;
	SdtNode *new_node = (SdtNode *)malloc(sizeof(SdtNode));
	if (new_node == NULL)
	{
		LOG("malloc error\n");
		return sdt_list;
	}

	memcpy(new_node, &temp_sdt_node, sizeof(SdtNode));
	new_node->next = NULL;

	if (current == NULL)
	{
		return new_node;
	}

	if (new_node->service_id < sdt_list->service_id)
	{
		new_node->next = sdt_list;
		return new_node;
	}

	while ((current->next != NULL) && (current->next->service_id < new_node->service_id))
	{
		current = current->next;
	}

	new_node->next = current->next;
	current->next  = new_node;

	return sdt_list;
}

int sdt_callback(Slot *slot, int filter_index, unsigned char *section_buffer, unsigned short pid)
{
	SectionHead      section_header    = {0};
	TableStatusNode *table_status_node = NULL;
	SdtNode          temp_sdt_node     = {0};

	int            max_read_position        = 0;
	int            read_position            = 0;
	unsigned short temp_transport_stream_id = 0;
	unsigned short temp_original_network_id = 0;
	int            descriptors_length       = 0;

	get_section_header(section_buffer, &section_header);
	if (section_header.section_length < SDT_HEADER_LENGTH + SDT_CRC_LENGTH)
	{
		return SDT_CALLBACK_SECTION_LENGTH_ERROR;
	}

	table_status_node = find_table_status_node_in_list(sdt_table_status_list, pid, section_header.table_id);
	if (table_status_node == NULL)
	{
		LOG("SDT table status node is NULL\n");
		return SDT_CALLBACK_NO_STATUS_ERROR;
	}

	if (is_table_status_node_complete(table_status_node) == 1)
	{
		return 0;
	}

	if (is_version_number_changed(table_status_node, section_header.version_number) == 1)
	{
		clear_sdt_list_by_table_id(sdt_list, section_header.table_id);
		table_status_node->version_number      = section_header.version_number;
		table_status_node->last_section_number = section_header.last_section_number;
		memset(table_status_node->mask, 0, sizeof(table_status_node->mask));
	}

	if (is_section_repeat(table_status_node, section_header.section_number) == 1)
	{
		return 0;
	}

	set_mask_by_section_number(table_status_node, section_header.section_number);

	temp_transport_stream_id = (section_buffer[3] << 8 )| section_buffer[4];
	temp_original_network_id = (section_buffer[8] << 8) | section_buffer[9];

	// LOG("\nsdt_callback:temp_transport_stream_id:0x%04x(%d), temp_original_network_id:0x%04x(%d)\n",
	//     temp_transport_stream_id, temp_transport_stream_id,
	//     temp_original_network_id, temp_original_network_id);

	read_position     = SDT_HEADER_LENGTH;
	max_read_position = section_header.section_length - SDT_CRC_LENGTH;

	while (read_position < max_read_position)
	{
		prase_sdt_info(section_buffer + read_position, &temp_sdt_node, section_header.table_id, temp_transport_stream_id, temp_original_network_id);
		descriptors_length = ((section_buffer[read_position + 3] & 0x0F) << 8) | section_buffer[read_position + 4];
		read_position += 5;

		// LOG("sdt_callback:     transport_stream_id:0x%04x(%d),      original_network_id:0x%04x(%d)\n",
		//     temp_sdt_node.transport_stream_id, temp_sdt_node.transport_stream_id,
		//     temp_sdt_node.original_network_id, temp_sdt_node.original_network_id);

		if (descriptors_length > 0)
		{
			temp_sdt_node.service_descriptor_list = parse_sdt_descriptor_info(section_buffer + read_position, descriptors_length);
			read_position += descriptors_length;
		}

		sdt_list = add_sdt_node_to_list(sdt_list, temp_sdt_node);
	}

	if (is_table_status_node_complete(table_status_node) == 1)
	{
		if (is_table_status_list_complete(sdt_table_status_list) == 1)
		{
			clear_filter(slot, filter_index);
			free_sdt_resource();
			set_sdt_channel_status();
			return 1;
		}
	}

	return 0;
}

void init_sdt_resource(Slot *slot)
{
	unsigned char sdt_filter_match[FILTER_MASK_LENGTH] = {0x47, 0x00, 0x11, 0x00, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char sdt_filter_mask[FILTER_MASK_LENGTH]  = {0xFF, 0x1F, 0xFF, 0x00, 0xFB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	unsigned char sdt_table_ids[2] = {0x42, 0x46};
	int           sdt_filter_index = 0;

	if (slot->ts_file == NULL)
	{
		LOG("Invalid parameters, error code : %d\n", SDT_INIT_PARAM_ERROR);
		return;
	}

	sdt_filter_index = alloc_filter(slot, sdt_filter_match, sdt_filter_mask, SDT_CRC_CHECK, sdt_callback);
	if (sdt_filter_index < 0)
	{
		LOG("alloc_filter error\n");
		return;
	}

	// add to status twice
	if (is_table_status_node_exist(sdt_table_status_list, SDT_PID, sdt_table_ids[0]) != 1)
	{
		sdt_table_status_list = add_table_status_node_to_list(sdt_table_status_list, SDT_PID, sdt_table_ids[0]);
	}

	if (is_table_status_node_exist(sdt_table_status_list, SDT_PID, sdt_table_ids[1]) != 1)
	{
		sdt_table_status_list = add_table_status_node_to_list(sdt_table_status_list, SDT_PID, sdt_table_ids[1]);
	}

	return;
}

void free_sdt_resource(void)
{
	free_table_status_list(sdt_table_status_list);
	sdt_table_status_list = NULL;
}

SdtList *get_sdt_list(void)
{
	return sdt_list;
}
void free_sdt_list(SdtList *sdt_list)
{
	SdtNode *next_sdt_node = NULL;
	while (sdt_list != NULL)
	{
		next_sdt_node = sdt_list->next;
		free(sdt_list);
		sdt_list = next_sdt_node;
	}

	return;
}

void printf_sdt_list(SdtList *sdt_list)
{
	SdtNode *current_node = sdt_list;
	if (current_node == NULL)
	{
		LOG("sdt_list is NULL\n");
		return;
	}

	DOUBLE_LINE
	LOG("SDT list:\n");
	while (current_node != NULL)
	{
		LOG("service_id: 0x%04X(%5d) | transport_stream_id: 0x%04X(%4d) | original_network_id :0x%04X(%4d)",
		    current_node->service_id, current_node->service_id,
		    current_node->transport_stream_id, current_node->transport_stream_id,
		    current_node->original_network_id, current_node->original_network_id);

		while (current_node->service_descriptor_list != NULL)
		{
			LOG("service_type: 0x%02X | service_provider_name: %s | service_name: %s \n",
			    current_node->service_descriptor_list->service_type,
			    current_node->service_descriptor_list->service_provider_name,
			    current_node->service_descriptor_list->service_name);
			current_node->service_descriptor_list = current_node->service_descriptor_list->next;
		}
		current_node = current_node->next;
	}
	DOUBLE_LINE
	return;
}
