#include "kademlia/KadNode.h"

// it is better to give a table/recomputation
uint8_t k_id_distance(uint64_t dis)
{
	uint8_t i = 0;
	while (dis)
	{ // 找到dis当中的最高位1所在倒数第几位
		dis = dis >> 1;
		i = i + 1;
	}
	return i - 1; // 返回序号，从0开始
}

// 计算异或距离
uint64_t id_distance(uint64_t xId, uint64_t yId)
{
	return xId ^ yId;
}

uint8_t initNodeTable(Deque **nodeTable)
{

	Deque *node = (Deque *)malloc(sizeof(Deque) * NUM_BUCKETS); // 分配空间
	if (node == NULL)
	{
		return 1;
	}
	*nodeTable = node;
	return 0;
}

void initLocalNode(Node *node)
{
	local_node = node;
	local_nodeId = node->id;
	read_cnt = 0;
}

// 用于快速排序的排序方式，升序
int dis_cmp(const void *a, const void *b)
{
	uint64_t dis_a = (*(const close_node *)a).dis;
	uint64_t dis_b = (*(const close_node *)b).dis;

	if (dis_a < dis_b)
		return -1;
	if (dis_a > dis_b)
		return 1;
	return 0;
}

// 在本地节点表中找到距离请求节点最近的几个节点返回给response，节点数记录在count
void find_node(Node *request, NodeList *response, uint8_t *count)
{
	// xil_printf("在find_node中, ip : %08x, node信息:\n", request->l_ip);
	// xil_printf("port %d\n", request->l_port);
	// printbuffer_len(request, sizeof(Node));
	// xil_printf("node addr: %p\n", request);
	uint64_t target_id = request->id; // 获取请求的id
	response[0] = *local_node;
	NodeList *nodes = (NodeList *)malloc(sizeof(NodeList) * K_CLOSET);
	findCloseById(target_id, nodes, count); // 根据target_id找到离它最近的几个node
	uint8_t i = 1;							// 用于迭代response数组的索引
	for (; i <= *count;)
	{
		// 将整个node_结构体赋值给response数组中的元素
		response[i] = nodes[i];
		// 增加索引
		i++;
	}
	*count = i;
#ifdef DHASH_DEBUG
	printf("find_nodes  %d\n", *count);
#endif
	freshNode(*request); // 将请求的node更新为本地最新访问的node
	free(nodes);
}

/* 根据key查找本地是否存在此key,存在则返回此节点以及个数为1
否则返回本地节点表中距离key最近的几个节点并返回给response，节点数保存到count*/
void find_value(uint64_t key, NodeList *response, uint8_t *value, uint8_t *count)
{
	*value = find_user(key, &hash_key);
	if (*value)
	{							   // 依据key在本地查找是否存在key
		response[0] = *local_node; // 找到了则将本地节点返回
		*count = 1;				   // 计数1
	}
	else
	{
		NodeList *nodes = (NodeList *)malloc(sizeof(Node) * K_CLOSET);
		findCloseById(key, nodes, count); // 查找最近的node
		uint8_t i = 0;					  // 用于迭代response数组的索引
		for (; i < *count;)
		{
			// 将整个nodes[i]结构体赋值给response数组中的元素
			response[i] = nodes[i];
			// 增加索引
			i++;
		}
		free(nodes);
	}
}

/* 刷新node，会将使用的节点更新到local_node 的nodetable的最前面，并删除多余的nodetable里的末尾node */
void freshNode(Node node)
{
	// xil_printf("fresh node, node ip %08x\n", node.l_ip);
	uint64_t target_id = node.id; // 需更新节点的id
	if (target_id == local_nodeId)
	{
		return;
	}
	uint64_t dis = id_distance(target_id, local_nodeId); // 计算目的和本地的节点的距离，异或
	uint64_t k_dis = k_id_distance(dis);				 // 计算dis最高位1所在位置
	uint8_t i;
	uint8_t size = getCurrentSize(nodetable + k_dis); // 第k_die层nodetable所含id个数的大小
	for (i = 0; i < size; i++)
	{
		if ((getIthNode(nodetable + k_dis, i))->id == target_id)
		{ // 当找到对应的id后，跳出
			break;
		}
	}
	if (i < size)
	{
		deleteIthNode(nodetable + k_dis, i); // 找到了id，擦除id对应的表项
	}
	else
	{
		if (total_nodes < MAX_NODES)
		{
			total_nodes++; // 是新加入的id且总个数未满，则计数加一
		}
	}
	// insert
	insertFront(nodetable + k_dis, node); // 在bucket里面重新加入node，表示最近被调用了（lru），这里插入如果是已经满了k_closet个，则会覆盖掉末尾的元素，因此不需要再做删除末尾元素
}

// 获取最接近本地节点的一组节点,虽然未涉及距离计算，但是bucket序号越靠前的nodetabl里的node距离越小
void closeNodes(NodeList *nodes, uint8_t *num_nodes)
{
	*num_nodes = 0;
	uint8_t size;
	for (uint8_t i = 0; i < NUM_BUCKETS; i++)
	{ // 遍历所有的buckets
		size = getCurrentSize(&nodetable[i]);
		for (int j = 0; j < size; j++)
		{ // 遍历当前bucket中的所有节点
			nodes[*num_nodes] = *getIthNode(&nodetable[i], j);
			*num_nodes = *num_nodes + 1; // 将当前节点添加到nodes双端队列中，并将num_nodes加1
			if (*num_nodes >= K_CLOSET)
			{ // 仅获取k_closet个节点
				return;
			}
		}
	}
}

/* 这个函数目的是从waitq中选择一个尚未被访问过的节点赋给node,成功则返回true
 否则返回false*/
bool pickNode(Node *node, NodeList *waitq, uint8_t num)
{
	for (uint8_t i = 0; i < num; i++)
	{
		if (find_user(waitq[i].id, &hash_visited))
		{
			continue;
		}
		else
		{
			*node = waitq[i];
			return true;
		}
	}
	return false; // 没有找到节点：如果所有的节点都已经被访问过，那么函数返回false。
}

// 根据target_id计算本地节点表中与key最近的节点并保存在nodes中，个数保存在num_nodes
void findCloseById(uint64_t target_id, NodeList *nodes, uint8_t *num_nodes)
{
	// nodes用于存储最终的结果，sorted用于临时存储计算过程中的结果
	//  再定义一个结构体，表示sorted,它可记录节点间的距离以及节点
	//  close_node sorted[total_nodes];								//定义一个total_nodes大小的结构体数组
	close_node *sorted = (close_node *)calloc(total_nodes, sizeof(close_node));
	uint8_t size, num;
	uint16_t i;
	num = 0;
	for (i = 0; i < NUM_BUCKETS; i++)
	{ // 遍历bucket，查找closet node的id
		size = getCurrentSize(&nodetable[i]);
		for (uint8_t j = 0; j < size; j++)
		{
			sorted[num].c_node = *(getIthNode(&nodetable[i], j));			 // 获得第j节点并赋给sorted
			sorted[num].dis = id_distance(sorted[num].c_node.id, target_id); // 根据本地id和target_id计算dis并赋给sorted
			// printbuffer_len(getIthNode(&nodetable[i], j), sizeof(Node) - 2);
			// printf("before sort, id = %016" PRIx64 ", dis = %lld\n", sorted[num].c_node.id, sorted[num].dis);
			num++;
		}
	}
	// qsort需要包含头文件 #include<stdlib.h>, 对sorted的num个几点按照dis排序，升序  注意！！！： 第三个参数是参与排序的元素的大小，这里虽然是依据结构体成员dis排序，但是大小是整个结构体的大小
	qsort(sorted, num, sizeof(close_node), dis_cmp); // 对sorted双端队列进行排序，排序的依据是dis的大小。
	// xil_printf("After sorting:\n");
	// for (i = 0; i < num; i++) {
	// 	printf("Node [%d]: id = %016" PRIx64 ", dis = %lld\n", i, sorted[i].c_node.id, sorted[i].dis);
	// }
	for (i = 0; i < num && i < K_CLOSET; i++)
	{ // 选择最接近的节点：从sorted中选择距离最小的几个节点，将这些节点添加到nodes节点队列中
		nodes[i] = sorted[i].c_node;
	}
	*num_nodes = i;
	free(sorted);
#ifdef DHASH_DEBUG
	xil_printf("fandCloseById, nums %d\n", i);
#endif
}

// 从local node的nodetable中删除target_id的node
void removeById(uint64_t target_id)
{
	uint64_t dis = id_distance(local_nodeId, target_id); // 计算两个id的dis
	uint8_t k_dis = k_id_distance(dis);					 // 找到target对应的本地bucket
	uint8_t i;
	uint8_t size = getCurrentSize(&nodetable[k_dis]);
	for (i = 0; i < size; i++)
	{
		if ((getIthNode(nodetable + k_dis, i))->id == target_id)
		{ // 当找到对应的id后，跳出
			break;
		}
	}
	if (i < size)
	{
		deleteIthNode(nodetable + k_dis, i); // 找到了id，擦除id对应的表项
		total_nodes--;						 // 总的node个数减一
	}
}

// 打印所有的nodetable的id
void printNodeTable()
{
	printf("=============nodetable============================\n");
	for (uint8_t i = 0; i < NUM_BUCKETS; i++)
	{
		uint8_t size = getCurrentSize(&nodetable[i]);
		if (size)
		{ // 第i层存在节点才会打印
			printf("The %dth bucket nodes: ", i);
			for (uint8_t j = 0; j < size; j++)
			{
				printf("node[%d] ip: %08x  ", j, getIthNode(nodetable + i, j)->l_ip);
			}
			printf("\n");
		}
	}
	printf("=============nodetable============================\n");
}

// 通知本地bucket下的所有nodetable中的其他节点本地节点即将退出
//  todo: 本节点的数据需要迁移到离本节点最近的节点中
void kad_exit()
{
	struct pbuf *udp_pld = alloc_udp_payload(4 + sizeof(Node));
	fillDhtHeader((u8 *)(udp_pld->payload), EXIT_REQ_DHT); // 填充DHT header，ox10是操作码
	// 生成udp报文，payload是请求退出的节点，转发给nodetable里的所有节点
	memcpy((udp_pld->payload + 4), local_node, sizeof(Node)); // 拷贝本地节点信息到payload
	Node *rm_node;
	for (uint64_t i = 0; i < NUM_BUCKETS; i++)
	{ // 遍历所有的bucket
		uint8_t size = getCurrentSize(&nodetable[i]);
		for (uint8_t j = 0; j < size; j++)
		{ // 通知每一个node，本节点要退出
			ip_addr_t ip_addr;
			rm_node = getIthNode(&nodetable[i], j); // 获取节点表的下的第j个节点
			ip_addr.addr = rm_node->l_ip;
			udp_sendto(udpecho_pcb, udp_pld, &ip_addr, rm_node->l_port); // 调用函数层层下发到网卡再发出
		}
	}
	pbuf_free(udp_pld);
}

/*
节点A收到节点C需要退出网络，节点A就删除本地保存的节点C的信息 */
void exit_rep(Node node)
{
	removeById(node.id); // 从节点表删除
	printNodeTable();
}

/*
新的节点主动向某个已存在的remote节点发送join请求 */
void join(Node *locla_node, Node *remote_node)
{
	struct pbuf *udp_pld = alloc_udp_payload(4 + sizeof(Node));
	fillDhtHeader((u8 *)(udp_pld->payload), 0x10);			  // 填充DHT header，ox10是操作码
	memcpy((udp_pld->payload + 4), local_node, sizeof(Node)); // 拷贝本地节点信息到payload
	// 从remote_node获取远端的ip和端口
	ip_addr_t ip_addr;
	ip_addr.addr = remote_node->l_ip; // 获取远端ip地址
	int error1 = 0;
	error1 = udp_sendto(udpecho_pcb, udp_pld, &ip_addr, remote_node->l_port); // 调用函数层层下发到网卡再发出
	pbuf_free(udp_pld);														  // 释放分配的空间
	xil_printf("Send join request to server!, server ip:%08x, error1 %d\n", remote_node->l_ip, error1);
}

/*
已存在的节点A接收到某个节点H需要加入的请求，返回H节点离A节点的nodetable中最近的不超过k_closet个节点 */
void join_req(Node *node)
{
	xil_printf("接收到join的节点, ip : %08x, node信息:\n", node->l_ip);
	xil_printf("port %d\n", node->l_port);
	printbuffer_len(node, sizeof(Node));
	// xil_printf("node addr: %p\n", node);
	NodeList *response = (NodeList *)malloc(sizeof(NodeList) * (K_CLOSET + 1)); //+1是因为还有本地节点也需要返回
	uint8_t count;
	// xil_printf("node addr: %p\n", node);
	find_node(node, response, &count); // 在本地节点表找到k_closet个距离node最近的节点，返回在response中
	// 将response组装成udp payload,需要一个字节记录返回了多少个node(*count),调用udp_send函数发送给node
	struct pbuf *udp_pld = alloc_udp_payload(4 + 1 + count * sizeof(Node));
	fillDhtHeader((u8 *)(udp_pld->payload), 0x11);					  // 填充DHT header，ox11是操作码
	*((u8 *)(udp_pld->payload) + 4) = count;						  // node个数
	memcpy((udp_pld->payload + 5), response, sizeof(Node) * (count)); // 拷贝节点信息到payload
	ip_addr_t ip_addr;
	ip_addr.addr = node->l_ip; // 获取远端ip地址
	// xil_printf("node addr: %p\n", node);
	int error1 = udp_sendto(udpecho_pcb, udp_pld, &ip_addr, node->l_port); // 调用函数层层下发到网卡再发出
	xil_printf("向加入节点回复！, ip:%08x %08x, error1 %d\n", node->l_ip, ip_addr.addr, error1);
	printbuffer_len(response, sizeof(Node) * (count));
	pbuf_free(udp_pld);
	free(response); // 释放分配的空间
	printNodeTable();
}

/*
新的节点在发出join请求后收到回复将回复中的节点加入到本地 */
void join_rep(NodeList *remote_nodes, uint8_t count)
{
	xil_printf("Add the server returned nodes in local node, nums %d\n", count);
	while (count--)
	{
		freshNode(remote_nodes[count]); // 将回复response的节点以及离local最近的nodes加入到本地
	}
	printNodeTable();
}

/*
在接收到RDMA read报文时调用的函数,由于read报文只包含BTH+RETH+CRC,总共12+16+4=32B,因此该报文也会被转发
如果本地存在key，则直接由本地处理packet
否则在离key最近的节点中寻找 */
uint8_t get(uint64_t key, uint8_t *packet)
{
	if (find_user(key, &hash_key))
	{ // 查找到了
		// xil_printf("Read in local node!\n");
		return 1; // 返回1，表示由本地处理报文
	}
	ip_addr_t ip_addr;
	hash_visited = NULL; // 先将存储被访问过的节点的表清空，防止以前遗留的信息干扰
	// 本地没找到就需要发给远端继续寻找
	Node next_node;
	NodeList *nodes = (Node *)malloc(sizeof(Node) * K_CLOSET);
	if (unlikely(nodes == NULL))
	{
		xil_printf("get: malloc failed\n");
	}
	// if ((read_cnt % 100) == 0 || read_cnt > 1500) {
	// 	printf("malloc :%p\n", nodes);
	// }
	uint8_t count;
	closeNodes(nodes, &count);
	if (pickNode(&next_node, nodes, count) == false)
	{						 // pickNode会从nodes中获取一个未被访问的node并赋给next_node
		hash_visited = NULL; // 如果都访问过了，将存储已经访问过的节点的表清空
		printf("no nodes to send 1, can not find node!\n");
		free(nodes);
		ip_addr.addr = client_ip_addr;
		send_rdmaCtrl_reply(&ip_addr, client_udp_port, 0); // 返回ip为0的payload
		return 0;
	}

	add_user(next_node.id, &hash_visited); // 将next_node.id加入已被访问的节点中

	// 将本节点信息、packet(key和len)组装成udp payload，然后发出给next_node 生成get_req的请求
	struct pbuf *udp_pld = alloc_udp_payload(4 + sizeof(Node) + CONTROL_PKT_LEN); // 由于packet包含key和len，总共12B
	fillDhtHeader((u8 *)(udp_pld->payload), GET_REQ_DHT);						  // 填充DHT header，操作码0x30
	memcpy((udp_pld->payload + 4), local_node, sizeof(Node));					  // 拷贝本地节点信息到payload
	memcpy(udp_pld->payload + 4 + sizeof(Node), packet, CONTROL_PKT_LEN);		  // 拷贝控制路径的信息

	ip_addr.addr = next_node.l_ip; // 获取远端ip地址
	char error_send = 0;
	error_send = udp_sendto(udpecho_pcb, udp_pld, &ip_addr, 8081); // 调用函数层层下发到网卡再发出,由于存储节点都是8081的端口号，这里就直接指定为8081
	// printbuffer_len(udp_pld->payload, udp_pld->tot_len);
	if (unlikely(error_send))
	{
		xil_printf("get: udp_sendto error %d\n", error_send);
	}

#ifdef DHASH_DEBUG
	xil_printf("get ,to next node's ip: %08x\n", ip_addr.addr);
#endif
	read_cnt++;
	// if ((read_cnt % 100) == 0) {
	// 	printf("g %d ", read_cnt);
	// }
	// printf("g %d ", read_cnt);

	pbuf_free(udp_pld); // 释放空间
	free(nodes);
	return 0;
}

/*
节点收到远端节点发起get操作，
会在本地查找key，存在则处理packet
不存在则返回一些与key相近的节点信息 */
void get_req(uint64_t key, uint8_t *packet, Node *req_node, uint16_t udp_port)
{
	// xil_printf("接收到发起get操作的节点, ip : %08x\n", req_node->l_ip);

	NodeList *response = (NodeList *)malloc(sizeof(Node) * K_CLOSET);
	if (unlikely(response == NULL))
	{
		xil_printf("get_req: malloc failed\n");
	}
	// if ((read_cnt % 100) == 0 || read_cnt > 1500) {
	// 	printf("malloc q :%p\n", response);
	// }
	uint8_t found_key = 0;
	uint8_t node_count = 0;
	find_value(key, response, &found_key, &node_count); // 在本地查找，未找到则返回一些与key相近的节点信息
	freshNode(*req_node);								// 更新发起request的node
	if (found_key)
	{
		rdma_key = key;
		rdma_value_len = *((uint32_t *)(packet + 8));
		rdma_ctrl_op = RD_CTRL_PATH;
		rdma_ctrl_loaclProcess = 1;
#ifdef DHASH_DEBUG
		printf("get op, obtain remote transmit control data, key : %016" PRIx64 "  %lu \n", rdma_key, rdma_value_len);
#endif
	}

	// 将response以及控制路径packet返回给发起get的节点，组装在udp->payload中，并设置一个字节指示本次是否已找到key（*found_key）,一个字节记录返回了多少个node(*node_count)
	struct pbuf *udp_pld = alloc_udp_payload(4 + 1 + 1 + node_count * sizeof(Node) + CONTROL_PKT_LEN);
	fillDhtHeader((u8 *)(udp_pld->payload), GET_RSP_DHT);				 // 填充DHT header，操作码0x31
	*((u8 *)(udp_pld->payload) + 4) = found_key;						 // 是否已找到key
	*((u8 *)(udp_pld->payload) + 5) = node_count;						 // 返回node个数
	memcpy((udp_pld->payload + 6), response, node_count * sizeof(Node)); // 拷贝节点信息到payload
	if (!found_key)
	{																						 // 本地未找到，需要将控制路径报文一起返回
		memcpy((udp_pld->payload + 6 + node_count * sizeof(Node)), packet, CONTROL_PKT_LEN); // 拷贝rdma控制报文
	}

	ip_addr_t ip_addr;
	ip_addr.addr = req_node->l_ip; // 获取远端ip地址
	char error_send = 0;
	error_send = udp_sendto(udpecho_pcb, udp_pld, &ip_addr, udp_port); // 调用函数层层下发到网卡再发出
	if (unlikely(error_send))
	{
		xil_printf("get_req: udp_sendto error %d\n", error_send);
	}
	// xil_printf("回复发起get操作的节点, 目的ip: %08x\n", ip_addr.addr);
	pbuf_free(udp_pld); // 释放分配的pbuf空间
	free(response);
	read_cnt++;

	// printf("q %d ", read_cnt);

	// free(found_key);
}

/*
节点收到远端节点对本端发起get操作的回复，
根据参数found判断是否已找到key的value
若已找到，则将存储已经访问过的节点的表清空，否则根据返回的节点信息继续寻找 */
void get_rsp(uint8_t found, uint8_t count, NodeList *rsp_node, uint8_t *packet, uint16_t udp_port)
{
	// xil_printf("New nodes count:%d\n", rsp_node->l_ip, count);
	ip_addr_t ip_addr;
	if (found)
	{							// 是
		freshNode(rsp_node[0]); // 更新回复response的node
		hash_visited = NULL;
		// 给client返回存储节点的ip
		ip_addr.addr = client_ip_addr;
		send_rdmaCtrl_reply(&ip_addr, client_udp_port, rsp_node[0].l_ip);
#ifdef DHASH_DEBUG
		xil_printf("get key find storage node, node ip:%08x\n", rsp_node[0].l_ip);
#endif
		return;
	}
	else
	{
		// printf("Don't find, continuing\n");
		for (uint8_t i = 0; i < count; ++i)
		{							// 没找到value，则将response中和key距离近的nodes加入到本地的nodetable中，因为前面的节点没找到，
			freshNode(rsp_node[i]); // 新加入的nodes里未寻找过，将其加入bucket的nodetable中
		}
	}

	Node next_node;
	NodeList *nodes = (Node *)malloc(sizeof(Node) * K_CLOSET);
	if (unlikely(nodes == NULL))
	{
		xil_printf("get_rsp: malloc failed\n");
	}
	// printf("malloc p :%p\n", nodes);

	closeNodes(nodes, &count);

	if (pickNode(&next_node, nodes, count) == false)
	{ // pickNode会从nodes中获取一个未被访问的node并赋给next_node
		hash_visited = NULL;
		printf("no nodes to send 2, can not find node!\n");
		free(nodes); // 如果都访问过了，将存储已经访问过的节点的表清空,并返回一个未找到的标识包给pc
		ip_addr.addr = client_ip_addr;
		send_rdmaCtrl_reply(&ip_addr, client_udp_port, 0); // 返回ip为0的payload
		return;
	}
	add_user(next_node.id, &hash_visited); // 将next_node.id加入已被访问的节点中

	// 将本节点信息、packet组装成udp payload，然后发出给next_node 生成get_req的请求
	struct pbuf *udp_pld = alloc_udp_payload(4 + sizeof(Node) + CONTROL_PKT_LEN); // 由于packet包含key和len，总共12B
	fillDhtHeader((u8 *)(udp_pld->payload), GET_REQ_DHT);						  // 填充DHT header，操作码0x30
	memcpy((udp_pld->payload + 4), local_node, sizeof(Node));					  // 拷贝本地节点信息到payload
	memcpy(udp_pld->payload + 4 + sizeof(Node), packet, CONTROL_PKT_LEN);		  // 拷贝控制路径的信息

	ip_addr.addr = next_node.l_ip; // 获取远端ip地址
	char error_send = 0;
	error_send = udp_sendto(udpecho_pcb, udp_pld, &ip_addr, udp_port); // 调用函数层层下发到网卡再发出
	if (unlikely(error_send))
	{
		xil_printf("get_rsp: udp_sendto error %d\n", error_send);
	}
	xil_printf("Continue to forward rd control packets to other nodes, ip: %08x\n", ip_addr.addr);

#ifdef DHASH_DEBUG
	xil_printf("Continue to forward rd packets to other nodes, ip: %08x\n", ip_addr.addr);
#endif
	pbuf_free(udp_pld);
	free(nodes); // 释放空间
}

/*
计算key和本地节点以及本地nodetable节点的距离，最小距离的节点存储key,value */
uint8_t put(uint64_t key, uint8_t *packet)
{
	Node target_node = *local_node;
	uint64_t target_dis = id_distance(target_node.id, key); // localnodeId和key异或

	for (uint8_t i = 0; i < NUM_BUCKETS; i++)
	{ // 遍历bucket												//对bucket i上锁
		uint8_t size = getCurrentSize(&nodetable[i]);
		for (uint8_t j = 0; j < size; j++)
		{
			Node tmp = *getIthNode(nodetable + i, j);
			uint64_t dis = id_distance(tmp.id, key); // 计算key和每个node的距离
			// printf("new dis : %016" PRIx64 " \n ", dis);
			if (dis < target_dis)
			{ // 如果存在比本地node更近的，则更新target_node和target_dis
				target_dis = dis;
				target_node = tmp;
			}
		}
	}
	if (local_nodeId == target_node.id)
	{ // 如果是存储在本地节点
		// xil_printf("Store on the local node!\n");
		add_user(key, &hash_key); // 直接调用函数存储key
		return 1;				  // 返回1，由本地继续处理报文
	}
	else
	{ // 否则需要通过转发存入其他node
		ip_addr_t ip_addr;
		char error_send = 0;
		// 向client返回存储节点的ip
		ip_addr.addr = client_ip_addr;
		send_rdmaCtrl_reply(&ip_addr, client_udp_port, target_node.l_ip);

		// 将本地节点和控制路径报文转发给target_node，调用udp函数发送，payload需要加上转发报文的标识，然后加上packet
		// 将本节点信息、packet组装成udp payload，然后发出给next_node 生成put_req的请求
		struct pbuf *udp_pld = alloc_udp_payload(4 + sizeof(Node) + CONTROL_PKT_LEN);
		fillDhtHeader((u8 *)(udp_pld->payload), PUT_REQ_DHT);					// 填充DHT header，操作码0x40
		memcpy((udp_pld->payload + 4), local_node, sizeof(Node));				// 拷贝本地节点信息到payload
		memcpy((udp_pld->payload + 4 + sizeof(Node)), packet, CONTROL_PKT_LEN); // 拷贝报文(len 是rdma报文数据长度)
		ip_addr.addr = target_node.l_ip;										// 获取远端ip地址
		error_send = udp_sendto(udpecho_pcb, udp_pld, &ip_addr, 8081);			// 调用函数层层下发到网卡再发出
		if (unlikely(error_send))
		{
			xil_printf("put: udp_sendto error %d\n", error_send);
		}
		// printbuffer_len(udp_pld->payload, udp_pld->tot_len);
		pbuf_free(udp_pld);

		freshNode(target_node); // 更新目的节点
#ifdef DHASH_DEBUG
		xil_printf("Put operation, send wr packets to remote node, ip: %08x\n", ip_addr.addr);
		// printNodeTable();
#endif
		return 0;
	}
}

/*
将远端节点转发的write/put控制路径的报文进行处理，这里就是存储key */
void put_rsp(uint64_t key, Node remote_node, uint32_t length)
{
	rdma_key = key;
	rdma_value_len = length;
	rdma_ctrl_op = WR_CTRL_PATH;

	add_user(key, &hash_key); // 记录下key
	freshNode(remote_node);	  // 记录远端节点到本地节点表
							  // rdma_ctrl_loaclProcess = 1;
#ifdef DHASH_DEBUG
	printf("obtain remote transmit control data, key : %016" PRIx64 "  %lu \n", rdma_key, rdma_value_len);
	printNodeTable();
#endif
}

void fillDhtHeader(uint8_t *payload, uint8_t opcode)
{
	memcpy(payload, DKR_STRING, 3);
	payload[3] = opcode;
}
