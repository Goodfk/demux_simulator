#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ts_global.h"
#include "slot_filter.h"
#include "pid_save.h"
#include "get_pat_info.h"
#include "get_pmt_info.h"
#include "get_sdt_info.h"
#include "get_eit_info.h"
#include "integrate_data.h"
#include "user.h"

void extract_packet_by_program_number(ProgramInfoList *program_info_list, FILE *input_fp, unsigned int start_Position, unsigned char packet_size, unsigned int program_number)
{
	ProgramInfoNode *current_program_info_node = NULL;
	PmtESList       *current_es_node           = NULL;
	FILE            *output_fp                 = NULL;

	unsigned short pid_array[256]        = {0};
	char           output_file_name[256] = {0};
	int            pid_array_count       = 0;
	int            packet_count          = 0;

	current_program_info_node = find_program_info_by_program_number(program_info_list, program_number);
	if (current_program_info_node == NULL)
	{
		printf("program_number is not exist in list\n");
		return;
	}

	snprintf(output_file_name, sizeof(output_file_name), "%s.ts%s", current_program_info_node->service_name, "\0");

	current_es_node = current_program_info_node->es_info_list;
	while (current_es_node != NULL)
	{
		printf("elementary_pid is 0x%04X(%d)\n", current_es_node->elementary_pid, current_es_node->elementary_pid);
		pid_array[pid_array_count] = current_es_node->elementary_pid;
		pid_array_count++;
		current_es_node = current_es_node->next;
	}
	LOG("pid_array_count : %d\n", pid_array_count);

	output_fp = fopen(output_file_name, "wb");
	if (output_fp == NULL)
	{
		printf("open output_file fail\n");
		return;
	}

	packet_count = extract_pid_packets(input_fp, start_Position, packet_size,
	                                   output_fp, pid_array, pid_array_count);
	LOG("packet_count : %d\n", packet_count);

	fclose(output_fp);
	return;
}

int process_input_buffer(char *input_buffer, unsigned int length)
{
	int temp_number = 0;

	if (strncmp(input_buffer, "exit", 4) == 0)
		return USER_EXIT;
	if (strncmp(input_buffer, "back", 4) == 0)
		return USER_BACK;
	if (strncmp(input_buffer, "next", 4) == 0)
		return USER_NEXT;
	if (strncmp(input_buffer, "prev", 4) == 0)
		return USER_PREV;
	if (strncmp(input_buffer, "save", 4) == 0)
		return USER_SAVE;

	temp_number = atoi(input_buffer);
	return temp_number;
}

int more_infomation_interface(ProgramInfoList *program_info_list, FILE *input_fp, unsigned int start_Position, unsigned char packet_size, int program_number)
{
	ProgramInfoNode *current_program_info_node      = NULL;
	char             input_buffer[MAX_INPUT_LENGTH] = {0};
	int              proess_return                  = -1;

	current_program_info_node = find_program_info_by_program_number(program_info_list, program_number);
	if (current_program_info_node == NULL)
	{
		printf("program_number is not exist in list\n");
		return -1;
	}

	printf_more_program_info(current_program_info_node);

	while (1)
	{
		printf("Please enter the operation you want to perform(prev, next, back, exit, save):");
		scanf("%s", input_buffer);
		printf("---------------------------------------------------------------------------------------------------------------\n");
		proess_return = process_input_buffer(input_buffer, MAX_INPUT_LENGTH);

		switch (proess_return)
		{
		case USER_EXIT:
			return USER_EXIT;

		case USER_BACK:
			return USER_BACK;

		case USER_NEXT:
			if (current_program_info_node->next != NULL)
			{
				current_program_info_node = current_program_info_node->next;
				printf_more_program_info(current_program_info_node);
			}
			else
			{
				printf("no next program_number\n");
			}
			break;

		case USER_PREV:
			if (current_program_info_node->prev != NULL)
			{
				current_program_info_node = current_program_info_node->prev;
				printf_more_program_info(current_program_info_node);
			}
			else
			{
				printf("no prev program_number\n");
			}
			break;

		case USER_SAVE:
			extract_packet_by_program_number(program_info_list, input_fp, start_Position, packet_size, current_program_info_node->program_number);
			printf("save file success\n");
			break;

		default:
			break;
		}
	}
}

void external_interface(FILE *input_fp, unsigned int start_Position, unsigned char packet_size)
{
	ProgramInfoList *program_info_list              = NULL;
	char             input_buffer[MAX_INPUT_LENGTH] = {0};
	int              proess_return                  = -1;
	int              program_number                 = 0;
LOG("111\n");
	program_info_list = get_program_info_list();

	while (proess_return != 0)
	{
		LOG("\n\n\n\n");
		printf_program_list(program_info_list);
		printf("save, exit or input program_number to get more infomation:");

		scanf("%s", input_buffer);
		proess_return = process_input_buffer(input_buffer, MAX_INPUT_LENGTH);
		if (proess_return == USER_EXIT)
			break;

		if (proess_return > 0)
		{
			if (more_infomation_interface(program_info_list, input_fp, start_Position, packet_size, proess_return) == USER_EXIT)
				break;
		}
		else if (proess_return == USER_SAVE)
		{
			printf("Please input program_number to save:");
			scanf("%d", &program_number);
			extract_packet_by_program_number(program_info_list, input_fp, start_Position, packet_size, program_number);
		}
	}

	free_program_info_list(program_info_list);
	return;
}