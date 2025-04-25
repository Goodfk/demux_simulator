/**
 * @file get_pmt_info.h
 *
 * @author :Yujin Yu
 * @date   :2025.04.02
 */
#ifndef GET_PMT_H
#define GET_PMT_H

#define PMT_TABLE_ID      0x02
#define PMT_HEADER_LENGTH 12
#define PMT_CRC_CHECK     1
#define PMT_CRC_LENGTH    4

#define MAX_SUBTITLING_INFO_COUNT        8
#define MAX_PRIVATE_DATA_SPECIFIER_COUNT 8
#define MAX_SUBTITLING_DESCRIPTOR_COUNT  8

// Enumeration of descriptor types
typedef enum
{
	DESCRIPTOR_PRIVATE_DATA_SPECIFIER = 0x5F,
	DESCRIPTOR_SUBTITLING             = 0x59,
	DESCRIPTOR_UNKNOWN                = 0xFF
} DescriptorType;

// Private data descriptor (0x5F)
typedef struct
{
	DescriptorType type;
	unsigned int   private_data_specifier; // 32
} PrivateDataSpecifierDescriptor;

// Subtitle information
typedef struct
{
	unsigned int   ISO_639_language_code; // 24
	unsigned char  subtitling_type;       // 8
	unsigned short composition_page_id;   // 16
	unsigned short ancillary_page_id;     // 16
} SubtitlingInfo;

// Subtitle descriptor (0x59)
typedef struct
{
	DescriptorType type;

	int            subtitling_info_count;
	SubtitlingInfo subtitling_info[MAX_SUBTITLING_INFO_COUNT];
} SubtitlingDescriptor;

// Descriptor set structure
typedef struct
{
	int                            private_data_specifier_count;
	PrivateDataSpecifierDescriptor private_data_specifiers[MAX_PRIVATE_DATA_SPECIFIER_COUNT];

	int                  subtitling_descriptor_count;
	SubtitlingDescriptor subtitling_descriptors[MAX_SUBTITLING_DESCRIPTOR_COUNT];
} DescriptorCollection;

// ES info
typedef struct PmtESNode
{
	unsigned char        stream_type;    // 8
	unsigned short       elementary_pid; // 13
	DescriptorCollection descriptors;
	struct PmtESNode    *next;
} PmtESNode, PmtESList;

// PMT node
typedef struct PmtNode
{
	unsigned short program_number; // 16 from pat

	unsigned short       pcr_pid; // 13
	DescriptorCollection program_descriptors;
	PmtESList           *es_info_list;
	struct PmtNode      *next;
} PmtNode, PmtList;

//--------------------------------------------------------------------------------------------
// Function declaration
//--------------------------------------------------------------------------------------------
int pmt_callback(Slot *slot, int filter_index, unsigned char *section_buffer, unsigned short pid);

void init_pmt_resource(Slot *slot, PatList *pat_list);
void free_pmt_resource(void);

PmtList *get_pmt_list(void);
void     printf_pmt_list(PmtNode *pmt_node);
void     free_pmt_list(PmtList *pmt_list);

PmtESList *add_pmt_es_node_to_list(PmtESList *pmt_es_list, PmtESNode temp_es_node);
void       free_pmt_es_list(PmtESList *pmt_es_list);

#endif
