/**
 * @file ts_global.h
 *
 * @author :Yujin Yu
 * @date   :2025.03.24
 */

#ifndef TS_GLOBAL_H
#define TS_GLOBAL_H

//--------------------------------------------------------------------------------------------
// macro definition
//--------------------------------------------------------------------------------------------
#define SYNC_BYTE 0x47 // Sync byte

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

// printf switch
#define ENABLE_PRINTF 1 // 1 open printf，0 close
#if ENABLE_PRINTF
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

#define SINGLE_LINE LOG("---------------------------------------------------------------------------------------------------------------\n");
#define DOUBLE_LINE LOG("===============================================================================================================\n");

// #define POSITION printf("%s -> function:%s, line: %d\n", __FILE__, __func__, __LINE__);
#define POSITION printf("%s:%d\n", __FILE__, __LINE__);

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct
{
	int is_pmt_finish;
	int is_sdt_finish;
} ChannelStatus;

// , error code : %d
enum
{
	OPEN_INPUT_FILE_ERROR = -1000,
	OPEN_OUTPUT_FILE_ERROR,

	FILTER_PARAM_ERROR = -900,
	FILTER_FSEEK_ERROR,

	DETECT_TS_PACKET_SIZE_PARAM_ERROR = -800,
	DETECT_TS_PACKET_SIZE_FSEEK_ERROR,
	DETECT_TS_PACKET_SIZE_FSEEK_BACK_ERROR,
	DETECT_TS_PACKET_SIZE_FILE_END,

	PAT_INIT_PARAM_ERROR = -700,
	PAT_INIT_ALLOC_FILTER_ERROR,
	PAT_CALLBACK_SECTION_LENGTH_ERROR,
	PAT_CALLBACK_NO_STATUS_ERROR,
	PAT_CALLBACK_PAT_LIST_NULL_ERROR,

	PMT_INIT_PARAM_ERROR = -600,
	PMT_INIT_ALLOC_FILTER_ERROR,
	PMT_CALLBACK_SECTION_LENGTH_ERROR,
	PMT_CALLBACK_NO_STATUS_ERROR,

	SDT_INIT_PARAM_ERROR = -500,
	SDT_MALLOC_SDT_INFO_ERROR,
	SDT_INIT_ALLOC_FILTER_ERROR,
	SDT_CALLBACK_SECTION_LENGTH_ERROR,
	SDT_CALLBACK_NO_STATUS_ERROR,
	SDT_CALLBACK_UNKNOWN_TABLE_ID_ERROR,

	EIT_INIT_PARAM_ERROR = -400,
	EIT_INIT_ALLOC_FILTER_ERROR,
	EIT_CALLBACK_SECTION_LENGTH_ERROR,
	EIT_CALLBACK_NO_STATUS_ERROR,

};

//--------------------------------------------------------------------------------------------
void set_pmt_channel_status(void);
void set_sdt_channel_status(void);
int  is_channel_status_finish(void);

#endif
