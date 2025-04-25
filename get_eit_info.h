/*
 * @file get_eit_info.h
 *
 * @author :Yujin Yu
 * @date   :2025.04.16
 */
#ifndef GET_EIT_INFO_H
#define GET_EIT_INFO_H

#define EIT_PID           0x0012
#define EIT_HEADER_LENGTH 14
#define EIT_CRC_CHECK     1
#define EIT_CRC_LENGTH    4

#define MAX_DESCRIPTOR_STRING_LENGTH 256

//---------------------------------------------------------------------0x4D
typedef struct ShortEventDescriptorNode
{
	unsigned int ISO_639_language_code; // 24
	char         name[MAX_DESCRIPTOR_STRING_LENGTH];
	char         text[MAX_DESCRIPTOR_STRING_LENGTH];

	struct ShortEventDescriptorNode *next;
} ShortEventDescriptorNode, ShortEventDescriptorList;

//---------------------------------------------------------------------0x4E
typedef struct ExtendedEventDescriptorNode
{
	unsigned char descriptor_number;      // 4
	unsigned char last_descriptor_number; // 4
	unsigned int  ISO_639_language_code;  // 24
	unsigned char item[MAX_DESCRIPTOR_STRING_LENGTH];
	unsigned char text[MAX_DESCRIPTOR_STRING_LENGTH];

	struct ExtendedEventDescriptorNode *next;
} ExtendedEventDescriptorNode, ExtendedEventDescriptorList;

//---------------------------------------------------------------------0x4F
typedef struct TimeShiftedEventDescriptorNode
{
	unsigned char  descriptor_length;    // 8
	unsigned short reference_service_id; // 16
	unsigned short reference_event_id;   // 16

	struct TimeShiftedEventDescriptorNode *next;
} TimeShiftedEventDescriptorNode, TimeShiftedEventDescriptorList;

//---------------------------------------------------------------------EitNode
typedef struct EitNode
{
	unsigned short service_id;                  // 16
	unsigned short transport_stream_id;         // 16
	unsigned short original_network_id;         // 16
	unsigned char  segment_last_section_number; // 8
	unsigned char  last_table_id;               // 8

	// loop
	unsigned short event_id;       // 16
	unsigned long  start_time;     // 40
	unsigned int   duration;       // 24
	unsigned char  running_status; // 3
	unsigned char  free_CA_mode;   // 1

	// descriptor of loop
	ShortEventDescriptorList       *short_event_descriptor_list;        // 0x4D
	ExtendedEventDescriptorList    *extended_event_descriptor_list;     // 0x4E
	TimeShiftedEventDescriptorList *time_shifted_event_descriptor_list; // 0x4F

	struct EitNode *next;
} EitNode, EitList;

//--------------------------------------------------------------------------------------------
// Function declaration
//--------------------------------------------------------------------------------------------
int eit_callback(Slot *slot, int filter_index, unsigned char *section_buffer, unsigned short pid);

void init_eit_resource(Slot *slot);
void free_eit_resource(void);

EitList *get_eit_list(void);
void     printf_eit_list(EitList *eit_list);
void     free_eit_list(EitList *eit_list);

ShortEventDescriptorList       *add_short_event_descriptor_node_to_list(ShortEventDescriptorList *short_event_descriptor_list,
                                                                        ShortEventDescriptorNode  temp_short_event_descriptor_node);
ExtendedEventDescriptorList    *add_extended_event_descriptor_node_list(ExtendedEventDescriptorList *extended_event_descriptor_list,
                                                                        ExtendedEventDescriptorNode  temp_extended_event_descriptor_node);
TimeShiftedEventDescriptorList *add_time_shifted_event_descriptor_node_list(TimeShiftedEventDescriptorList *time_shifted_event_descriptor_list,
                                                                            TimeShiftedEventDescriptorNode  temp_time_shifted_event_descriptor_node);

void free_short_event_descriptor_list(ShortEventDescriptorList *short_event_descriptor_list);
void free_extended_event_descriptor_list(ExtendedEventDescriptorList *extended_event_descriptor_list);
void free_time_shifted_event_descriptor_list(TimeShiftedEventDescriptorList *time_shifted_event_descriptor_list);

#endif