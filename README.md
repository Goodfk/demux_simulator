一、项目概述
	本项目是一个用于解析和处理 MPEG-2 传输流（TS）文件的工具集，
	主要功能包括解析节目关联表（PAT）、节目映射表（PMT）、服务描述表（SDT）和事件信息表（EIT）等。
	通过对这些表的解析，可以获取到 TS 流中的节目信息、服务信息以及事件信息等。


二、文件结构与功能
1. ts_analyzer.c
	功能：验证 TS 数据包的大小是否符合预期。
	关键函数：validate_packet_size

2. get_pmt_info.c
	功能：获取和处理 PMT 信息，包括初始化资源、解析 PMT 表、打印 PMT 列表等。
	关键函数：
	init_pmt_resource：初始化 PMT 资源。
	pmt_callback：处理 PMT 回调事件。
	printf_pmt_list：打印 PMT 列表信息。

3. get_eit_info.c
	功能：获取和处理 EIT 信息，包括初始化资源、解析 EIT 表、删除重复节点、打印 EIT 列表等。
	关键函数：
	init_eit_resource：初始化 EIT 资源。
	eit_callback：处理 EIT 回调事件。
	delete_nodes_in_eit_list_by_time：根据时间删除 EIT 列表中的节点。
	printf_eit_list：打印 EIT 列表信息。

4. get_sdt_info.c
	功能：获取和处理 SDT 信息，包括解析 SDT 表、添加 SDT 节点到列表等。
	关键函数：
	sdt_callback：处理 SDT 回调事件。
	parse_sdt_descriptor_info：解析 SDT 描述符信息。

5. get_pat_info.c
	功能：获取和处理 PAT 信息，包括解析 PAT 表、添加 PAT 节点到列表等。
	关键函数：
	pat_callback：处理 PAT 回调事件。

6. integrate_data.c
	功能：根据 PAT 和 PMT 信息初始化节目信息列表。
	关键函数：
	init_program_info_list：初始化节目信息列表。

7. parse_tables_status.c
	功能：处理表状态信息，包括判断节点是否存在、添加节点到列表、查找节点、判断列表是否完成等。
	关键函数：
	is_table_status_node_exist：判断表状态节点是否存在。
	add_table_status_node_to_list：添加表状态节点到列表。
	find_table_status_node_in_list：在列表中查找表状态节点。
	is_table_status_list_complete：判断表状态列表是否完成。

8. slot_filter.c
	功能：计算 CRC32 值和进行 CRC 验证。
	关键函数：
	calculate_crc32：计算 CRC32 值。
	crc_check：进行 CRC 验证。

9. main.c
	功能：程序入口，打开输入文件，调用处理函数，最后关闭文件。
	关键函数：
	main：程序主函数。


三、使用方法
1. 编译
	将所有的 .c 文件一起编译，例如使用以下命令：
	make clean
	make
	./test.exe

2. 运行
	在 main.c 中修改 input_file 为你要解析的 TS 文件的路径，然后运行编译后的可执行文件：

	四、注意事项
	确保输入的 TS 文件路径正确，并且程序有读取该文件的权限。
	在处理过程中，可能会因为文件格式不正确或其他原因导致解析失败，程序会输出相应的错误信息。
