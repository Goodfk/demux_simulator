#ifndef USER_H
#define USER_H

#define MAX_INPUT_LENGTH 256

enum USER_COMMAND
{
	USER_EXIT = 0,
	USER_BACK = -1,
	USER_NEXT = -2,
	USER_PREV = -3,
	USER_SAVE = -4,

};

void external_interface(FILE *input_fp, unsigned int start_Position, unsigned char packet_size);

#endif