#include "dequeue.h"

// 初始化双端队列
void initDeque(Deque *deque) {
    // printf("Initializing deque at address: %p\n", (void*)deque);
    deque->front = -1;
    deque->rear = 0;      
}

// 检查双端队列是否为空
bool isEmpty(Deque *deque) {
    return deque->front == -1;
}

// 检查双端队列是否已满
bool isFull(Deque *deque) {
    return (deque->front == 0 && deque->rear == MAX_SIZE - 1) || (deque->front == deque->rear + 1);
}

// 在队头插入元素
void insertFront(Deque *deque, Node node) {
    if (isEmpty(deque)) {
        deque->front = 0;
        deque->rear = 0;
    } else if (deque->front == 0) {
        deque->front = MAX_SIZE - 1;
    } else {
        deque->front--;
    }

    deque->nd_list[deque->front] = node;
    if (isFull(deque)) {            //在队列已经满时，需要修改队尾指针，否则对计数有影响。。
        deque->rear = (deque->rear - 1 + MAX_SIZE) % MAX_SIZE;
    }
}

// 在队尾插入元素
void insertRear(Deque *deque, Node node) {
    if (isFull(deque)) {
        return;
    }
    if (isEmpty(deque)) {
        deque->front = 0;
        deque->rear = 0;
    } else if (deque->rear == MAX_SIZE - 1) {
        deque->rear = 0;
    } else {
        deque->rear++;
    }

    deque->nd_list[deque->rear] = node;
}

// 从离front距离i的位置删除元素
void deleteIthNode(Deque *deque, uint8_t i) {
    if (isEmpty(deque) || i < 0 || i > (deque->rear - deque->front + MAX_SIZE) % MAX_SIZE) {
        // 队列为空或 i 无效
        printf("Invalid index or deque is empty.\n");
        return; // 表示错误或队列空
    }
    //如果只有一个元素，就不需要拷贝,将其置为空数组
    if (deque->front == deque->rear) {
        deque->front = -1;
        deque->rear = 0;
        return;
    }
    // 计算第 i 个数据的索引
    int index = (deque->front + i) % MAX_SIZE;
    int j = (index + 1) % MAX_SIZE;
    //将后面的值移到前面
    do {
        deque->nd_list[index] = deque->nd_list[j];
        index = (index + 1) % MAX_SIZE;
        j = (j + 1) % MAX_SIZE;
    } while (j != deque->rear);

    deque->rear = (deque->rear - 1 + MAX_SIZE) % MAX_SIZE;      //更新尾部位置
}


Node* getIthNode(Deque *deque, int i) {
    /* Node* tmp;
    tmp = NULL;
    if (isEmpty(deque) || i < 0 || i > (deque->rear - deque->front + MAX_SIZE) % MAX_SIZE) {
        // 队列为空或 i 无效
        printf("Invalid index or deque is empty.\n");
        return tmp;  // 返回一个表示错误的值，你可以根据实际需求返回适当的错误值
    } */   //暂时可以不判定，因为传入的参数i一定满足

    // 计算第 i 个数据的索引
    int index = (deque->front + i) % MAX_SIZE;
    return &(deque->nd_list[index]);
}

// 获取当前队列中的元素个数
int getCurrentSize(Deque *queue) {
    if (isEmpty(queue)) {
        return 0;
    } else if (queue->front <= queue->rear) {       
        return queue->rear - queue->front + 1;
    } else {                                    //rear比front小时，计数不一样
        return MAX_SIZE - (queue->front - queue->rear - 1);
    }
}




