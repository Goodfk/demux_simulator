#include <stdio.h>
#include "ts_global.h"

static ChannelStatus channel_status = {0};

void set_pmt_channel_status(void)
{
	channel_status.is_pmt_finish = 1;
}

void set_sdt_channel_status(void)
{
	channel_status.is_sdt_finish = 1;
}

int is_channel_status_finish(void)
{
	if (1 == (channel_status.is_pmt_finish && channel_status.is_sdt_finish))
		return 1;

	return 0;
}