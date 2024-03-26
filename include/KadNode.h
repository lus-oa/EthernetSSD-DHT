#ifndef KADNODE_H
#define KADNODE_H

#include <math.h>
#include <stddef.h>
#include "dequeue.h"
#include "hash_map.h"

#include "udp_echo.h"

#define NUM_BUCKETS 64
#define K_CLOSET 7
#define U64_MAX
#define MAX_NODES (NUM_BUCKETS * K_CLOSET)

// #define DHASH_DEBUG

uint32_t total_nodes;
uint64_t id_distance(uint64_t xId, uint64_t yId);

// it is better to give a table/recomputation
uint8_t k_id_distance(uint64_t dis);

uint64_t local_nodeId;
Node *local_node;
// 需要本节点对控制路径报文处理的标志
uint8_t rdma_ctrl_loaclProcess;

// 控制报文里的key和value长度
uint64_t rdma_key;
uint32_t rdma_value_len;
uint8_t rdma_ctrl_op;

// NodeTable, need design.
Deque *nodetable;

struct my_hash *hash_key;
struct my_hash *hash_visited;

uint8_t initNodeTable(Deque **nodeTable);

void initLocalNode(Node *node);

int dis_cmp(const void *a, const void *b);

void find_node(Node *request, NodeList *response, uint8_t *count);

/* 根据key查找本地是否存在此key,存在则返回此节点以及个数为1
否则返回本地节点表中距离key最近的几个节点并返回给response，节点数保存到count*/
void find_value(uint64_t key, NodeList *response, uint8_t *value, uint8_t *count);

/* 刷新node，会将使用的节点更新到local_node 的nodetable的最前面，并删除多余的nodetable里的末尾node */
void freshNode(Node node);

// 获取最接近本地节点的一组节点,虽然未涉及距离计算，但是bucket序号越靠前的nodetabl里的node距离越小
void closeNodes(NodeList *nodes, uint8_t *num_nodes);

/* 这个函数目的是从waitq中选择一个尚未被访问过的节点赋给node,成功则返回true
 否则返回false*/
bool pickNode(Node *node, NodeList *waitq, uint8_t num);

// 根据target_id计算本地节点表中与key最近的节点并保存在nodes中，个数保存在num_nodes
void findCloseById(uint64_t target_id, NodeList *nodes, uint8_t *num_nodes);

// 从local node的nodetable中删除target_id的node
void removeById(uint64_t target_id);

// 打印所有的nodetable的id
void printNodeTable();
void kad_exit();
void exit_rep(Node node);
void join(Node *locla_node, Node *remote_node);
void join_req(Node *node);
void join_rep(NodeList *remote_nodes, uint8_t count);
uint8_t get(uint64_t key, uint8_t *packet);
void get_req(uint64_t key, uint8_t *packet, Node *req_node, uint16_t udp_port);
void get_rsp(uint8_t found, uint8_t count, NodeList *rsp_node, uint8_t *packet, uint16_t udp_port);
uint8_t put(uint64_t key, uint8_t *packet);
void put_rsp(uint64_t key, Node remote_node, uint32_t length);
void fillDhtHeader(uint8_t *payload, uint8_t opcode);

int read_cnt;

#endif /* KADNODE_H */

/* 双端队列（Double-ended queue，简称Deque）是一种特殊的队列，它允许我们在队列的两端进行插入和删除操作。这意味着元素可以从队头出队和入队，也可以从队尾出队和入队。
以下是双端队列的一些关键特性：
1. **两端操作**：与普通队列（只允许在一端插入，在另一端删除）和栈（只允许在同一端插入和删除）不同，双端队列允许在两端进行插入和删除操作。

2. **灵活性**：由于双端队列允许在两端进行操作，因此它比普通队列和栈更加灵活。例如，如果我们只使用队尾进行插入和删除操作，那么双端队列就可以作为栈来使用。
如果我们在队尾插入元素，在队头删除元素，那么双端队列就可以作为普通队列来使用。

3. **内部实现**：双端队列通常可以通过数组或链表来实现。如果使用数组实现，为了充分利用数组空间，通常会使用取模运算来实现逻辑上的循环数组。 */
