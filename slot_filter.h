/**
 * @file slot_filter.h
 *
 * @author :Yujin Yu
 * @date   :2025.04.14
 */
#ifndef SLOT_FILTER_H
#define SLOT_FILTER_H

//---------------------------------------------------------------------------------------------------------------------

#define FILTER_MASK_LENGTH 16
#define MAX_SECTION_LENGTH 4096
#define MAX_FILTER_COUNT   32

//---------------------------------------------------------------------------------------------------------------------
typedef struct
{
	unsigned sync_byte                   : 8;
	unsigned transport_error_indicator   : 1;
	unsigned payload_unit_start_indicator: 1;
	unsigned transport_priority          : 1;
	unsigned PID                         : 13;
	unsigned transport_scrambling_control: 2;
	unsigned adaptation_field_control    : 2;
	unsigned continuity_counter          : 4;
} TSPacketHead; // total 4byte

typedef struct
{
	unsigned table_id           : 8;
	unsigned section_length     : 12;
	unsigned version_number     : 5;
	unsigned section_number     : 8;
	unsigned last_section_number: 8;
} SectionHead;

typedef struct Slot   Slot;
typedef struct Filter Filter;

typedef int (*parse_callback)(Slot *slot, int filter_index, unsigned char *section_buffer, unsigned short pid);

struct Filter
{
	int            is_used;
	unsigned char  filter_match[FILTER_MASK_LENGTH];
	unsigned char  filter_mask[FILTER_MASK_LENGTH];
	parse_callback section_callback;
	int            is_CRC_check;
	int            is_write_flag;
	unsigned short section_length;
	unsigned char  section_buffer[MAX_SECTION_LENGTH];
	unsigned short payload_length_count;
};

struct Slot
{
	FILE         *ts_file;
	unsigned char packet_size;
	long          start_position;
	Filter        filter_array[MAX_FILTER_COUNT];
};

//---------------------------------------------------------------------------------------------------------------------
Slot init_slot(FILE *ts_file, unsigned char packet_size, unsigned int start_position);
void clear_slot(Slot *slot);

/**
 * @brief Allocate a filter slot
 *
 * @param slot             Pointer to the Slot structure
 * @param filter_match     Filter match pattern
 * @param filter_mask      Filter mask
 * @param is_crc_check     Flag to indicate if CRC check is required
 * @param section_callback Callback function for section processing
 *
 * @return >0 :Index of the allocated filter
 *         <0 :failure, no filter available
 */
int  alloc_filter(Slot *slot, unsigned char *filter_match, unsigned char *filter_mask, int is_crc_check, parse_callback section_callback);
void clear_filter(Slot *slot, int index);

int  section_filter(Slot *slot);
void get_section_header(unsigned char *buffer, SectionHead *section_header);

#endif
