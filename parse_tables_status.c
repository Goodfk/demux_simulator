/**
 * @file parse_tables_status.c
 *
 * @author :Yujin Yu
 * @date   :2025.04.10
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ts_global.h"
#include "parse_tables_status.h"

int is_table_status_node_exist(TableStatusList *table_status_list, unsigned short pid, unsigned char table_id)
{
	TableStatusNode *current_node = table_status_list;

	while (current_node != NULL)
	{
		if ((current_node->pid == pid) && (current_node->table_id == table_id))
		{
			return 1;
		}
		current_node = current_node->next;
	}

	return 0;
}

TableStatusList *add_table_status_node_to_list(TableStatusList *table_status_list, unsigned short pid, unsigned char table_id)
{
	TableStatusNode *table_status_node = (TableStatusNode *)malloc(sizeof(TableStatusNode));
	if (table_status_node == NULL)
	{
		LOG("malloc error\n");
		return table_status_list;
	}

	memset(table_status_node, 0, sizeof(TableStatusNode));
	table_status_node->pid                 = pid;
	table_status_node->table_id            = table_id;
	table_status_node->version_number      = -1;
	table_status_node->last_section_number = 0;
	table_status_node->next                = table_status_list;

	return table_status_node;
}

TableStatusNode *find_table_status_node_in_list(TableStatusList *table_status_list, unsigned short pid, unsigned char table_id)
{
	TableStatusNode *current_node = table_status_list;

	while (current_node != NULL)
	{
		if ((current_node->pid == pid) && (current_node->table_id == table_id))
		{
			return current_node;
		}
		current_node = current_node->next;
	}

	return NULL;
}

int is_version_number_changed(TableStatusNode *table_status_node, unsigned char version_number)
{
	if (table_status_node->version_number != version_number)
		return 1;

	return 0;
}

int is_section_repeat(TableStatusNode *table_status_node, unsigned char section_number)
{
	int index    = section_number / 32;
	int mask_bit = 1 << (section_number % 32);

	if ((table_status_node->mask[index] & mask_bit) != 0)
		return 1;

	return 0;
}

void set_mask_by_section_number(TableStatusNode *table_status_node, unsigned char section_number)
{
	int index    = section_number / 32;
	int mask_bit = 1 << (section_number % 32);

	table_status_node->mask[index] |= mask_bit;
}

int is_table_status_node_complete(TableStatusNode *table_status_node)
{
	int section_count = 0;
	int i = 0, j = 0;
	// clang-format off
	for (i=0; i<8; i++)
	{
		for (j=0; j<32; j++)
		{ // clang-format on
			if ((table_status_node->mask[i] & (1 << j)) == 1)
			{
				section_count++;
			}
		}
	}

	if (section_count >= table_status_node->last_section_number + 1)
		return 1;

	return 0;
}

int is_table_status_list_complete(TableStatusNode *table_status_list)
{
	if (table_status_list == NULL)
	{
		LOG("table status list is NULL\n");
		return 0;
	}

	TableStatusNode *current_node = table_status_list;

	while (current_node != NULL)
	{
		// if (current_node->is_complete == 0)
		if (is_table_status_node_complete(current_node) == 0)
		{
			return 0;
		}
		current_node = current_node->next;
	}

	return 1;
}

void free_table_status_list(TableStatusList *table_status_list)
{
	TableStatusNode *current_node = table_status_list;
	TableStatusNode *next_node    = NULL;

	while (current_node != NULL)
	{
		next_node = current_node->next;
		free(current_node);
		current_node = next_node;
	}
}
