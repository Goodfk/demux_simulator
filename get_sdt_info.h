#ifndef GET_SDT_INFO_H
#define GET_SDT_INFO_H

#define SDT_PID         0x0011
#define SDT_HEADER_LENGTH 11
#define SDT_CRC_CHECK     1
#define SDT_CRC_LENGTH    4


#define MAX_NAME_LENGTH 256

typedef struct ServiceDescriptorNode
{
	unsigned char                 service_type;   // 8
	unsigned char                 service_provider_name[MAX_NAME_LENGTH];
	unsigned char                 service_name[MAX_NAME_LENGTH];
	struct ServiceDescriptorNode *next;
} ServiceDescriptorNode, ServiceDescriptorList;

typedef struct SdtNode
{
	unsigned table_id           : 8;
	unsigned transport_stream_id: 16;
	unsigned original_network_id: 16;

	// loop
	unsigned               service_id                : 16;
	unsigned               EIT_schedule_flag         : 1;
	unsigned               EIT_present_following_flag: 1;
	unsigned               running_status            : 3;
	unsigned               free_CA_mode              : 1;
	ServiceDescriptorList *service_descriptor_list;
	struct SdtNode        *next;
} SdtList, SdtNode;


//--------------------------------------------------------------------------------------------
// Function declaration
//--------------------------------------------------------------------------------------------
int sdt_callback(Slot *slot, int filter_index, unsigned char *section_buffer, unsigned short pid);

void init_sdt_resource(Slot *slot);
void free_sdt_resource();

SdtList *get_sdt_list(void);
void     free_sdt_list(SdtList *sdt_list);
void     printf_sdt_list(SdtList *sdt_list);

#endif