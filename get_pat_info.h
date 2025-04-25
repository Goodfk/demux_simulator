/**
 * @file get_pat_info.h
 *
 * @author :Yujin Yu
 * @date   :2025.03.25
 */
#ifndef GET_PAT_INFO_H
#define GET_PAT_INFO_H

#define PAT_PID           0x0000
#define PAT_TABLE_ID      0x00
#define PAT_HEADER_LENGTH 8
#define PAT_CRC_CHECK     1
#define PAT_CRC_LENGTH    4

typedef struct PatNode
{
	unsigned short  transport_stream_id; // 16
	unsigned short  program_number;      // 16
	unsigned short  program_map_PID;     // 13
	struct PatNode *next;
} PatList, PatNode;

//--------------------------------------------------------------------------------------------
// Function declaration
//--------------------------------------------------------------------------------------------
int pat_callback(Slot *slot, int filter_index, unsigned char *section_buffer, unsigned short pid);

void init_pat_resource(Slot *slot);
void free_pat_resource(void);

PatList *get_pat_list(void);
void     free_pat_list(PatList *pat_list);
void     printf_pat_list(PatList *pat_list);

#endif
