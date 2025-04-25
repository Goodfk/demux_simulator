/**
 * @file parse_tables_status.h
 *
 * @author :Yujin Yu
 * @date   :2025.04.14
 */
#ifndef PSRSE_TABLES_STATUS_H
#define PSRSE_TABLES_STATUS_H

#define MASK_NUM 8 // 32*8=256,256 is the max number of section number

typedef struct TableStatusNode
{
	unsigned short          pid;                 // 13 before
	unsigned char           table_id;            // 8  before
	char                    version_number;      // 5
	unsigned char           last_section_number; // 8
	unsigned int            mask[MASK_NUM];
	struct TableStatusNode *next;
} TableStatusNode, TableStatusList;

// before get_section
int              is_table_status_node_exist(TableStatusList *table_status_list, unsigned short pid, unsigned char table_id);
TableStatusList *add_table_status_node_to_list(TableStatusList *table_status_list, unsigned short pid, unsigned char table_id);

// after get_section
TableStatusNode *find_table_status_node_in_list(TableStatusList *table_status_list, unsigned short pid, unsigned char table_id);

int  is_table_status_node_exist(TableStatusList *table_status_list, unsigned short pid, unsigned char table_id);
int  is_version_number_changed(TableStatusNode *table_status_node, unsigned char version_number);
int  is_section_repeat(TableStatusNode *table_status_node, unsigned char section_number);
void set_mask_by_section_number(TableStatusNode *table_status_node, unsigned char section_number);
int  is_table_status_node_complete(TableStatusNode *table_status_node);
int  is_table_status_list_complete(TableStatusNode *table_status_list);
void free_table_status_list(TableStatusList *table_status_list);

#endif