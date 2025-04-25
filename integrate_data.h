#ifndef INTEGRATE_DATA_H
#define INTEGRATE_DATA_H

typedef struct EventDataNode
{
	unsigned short event_id;   // eit
	unsigned long  start_time; // eit
	unsigned int   duration;   // eit

	ShortEventDescriptorList       *short_event_descriptor_list;        // eit
	ExtendedEventDescriptorList    *extended_event_descriptor_list;     // eit
	TimeShiftedEventDescriptorList *time_shifted_event_descriptor_list; // eit

	struct EventDataNode *next;
} EventDataNode, EventDataList;

typedef struct ProgramInfoNode
{
	unsigned short program_number; // pat, pmt, sdt(service_id), eit(service_id)

	unsigned short transport_stream_id; // pat, sdt, eit
	unsigned short original_network_id; //      sdt, eit

	unsigned short pcr_pid;      // pmt
	PmtESList     *es_info_list; // pmt

	unsigned char service_type;                           // sdt
	char          service_provider_name[MAX_NAME_LENGTH]; // sdt
	char          service_name[MAX_NAME_LENGTH];          // sdt

	EventDataList *event_data_list;

	struct ProgramInfoNode *prev;
	struct ProgramInfoNode *next;
} ProgramInfoNode, ProgramInfoList;

//--------------------------------------------------------------------------------------------
// Function declaration
//--------------------------------------------------------------------------------------------
ProgramInfoList *get_program_info_list(void);
void             free_program_info_list(ProgramInfoList *program_info_list);

ProgramInfoNode *find_program_info_by_program_number(ProgramInfoList *program_info_list, int program_number);

void printf_program_list(ProgramInfoList *program_info_list);
void printf_more_program_info(ProgramInfoNode *program_info_node);

#endif
