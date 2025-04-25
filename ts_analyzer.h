/**
 * @file ts_analyzer.h
 *
 * @brief The header file of TS file analysis tool
 *
 * @author :Yujin Yu
 * @date   :2025.03.05
 */

#ifndef TS_ANALYZER_H
#define TS_ANALYZER_H
//--------------------------------------------------------------------------------------------
// macro definition
//--------------------------------------------------------------------------------------------
#define VALIDATION_DEPTH    10 // Validation depth
#define TS_PACKET_SIZE      188
#define TS_DVHS_PACKET_SIZE 192
#define TS_FEC_PACKET_SIZE  204

//--------------------------------------------------------------------------------------------
// Function declaration
//--------------------------------------------------------------------------------------------
/**
 * @brief Detects the packet size of a TS (Transport Stream) packet.
 *
 * @param input_fp A pointer to the file to be analyzed.
 * @param first_sync_position A pointer to store the position of the first synchronization byte found.
 *
 * @return >0: packet size
 *         =0: found
 *         <0: error
 */
int detect_ts_packet_size(FILE *input_fp, long *first_sync_position);

#endif
