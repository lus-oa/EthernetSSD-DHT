#ifndef DEQUEUE_H
#define DEQUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_SIZE 7             // maximum size of queue, 目前由于一个bucket只有7个node

typedef struct {
    uint64_t id;                // ID of the node   64bit
    uint32_t l_ip;              // IP address   如果是c0 a8 01 0f，则在这里的值是0x0f01a8c0
    uint16_t l_port;            // udp Port      按照十进制赋值即可，比如4791=0x12b7
} Node, NodeList;

typedef struct {
    NodeList nd_list[MAX_SIZE];         // list of nodes
    int8_t front;                       // 指向队列头部
    int8_t rear;                        // 指向队列尾部的下一个位置
} Deque;

typedef struct {
    Node c_node;                        //保存节点         	        
    uint64_t dis;                       //与某节点的距离
} close_node;


// 初始化双端队列
void initDeque(Deque *deque);

// 检查双端队列是否为空
bool isEmpty(Deque *deque);

// 检查双端队列是否已满
bool isFull(Deque *deque);

// 在队头插入元素
void insertFront(Deque *deque, Node node);

// 在队尾插入元素
void insertRear(Deque *deque, Node node);

// 从离front距离i的位置删除元素
void deleteIthNode(Deque *deque, uint8_t i);


Node* getIthNode(Deque *deque, int i);

// 获取当前队列中的元素个数
int getCurrentSize(Deque *queue);


#endif