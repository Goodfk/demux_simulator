/**
 * @file pid_extractor.h
 *
 * @author :Yujin Yu
 * @date   :2025.03.11
 */

#ifndef PID_SAVE_H
#define PID_SAVE_H

int extract_pid_packets(FILE *input_fp, long start_Position, unsigned char packet_size,
                        FILE *fp_out, const unsigned short *pids_array, int array_count);

#endif
