#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stdio.h>   /* printf */
#include <stdlib.h>  /* atoi, malloc */
#include <string.h>  /* strcpy */
#include "uthash.h"  /* hash */
#include <stdint.h>  /* uint8_t... */
#include <stdbool.h> /* bool */

struct my_hash
{
    uint64_t id;       /* key */
    UT_hash_handle hh; /* makes this structure hashable 这个hh结构体就是哈希表里的bucket里的一个key,value，当然这个结构体还存在一些其他成员 */
};

void add_user(uint64_t user_id, struct my_hash **users);

uint8_t find_user(uint64_t user_id, struct my_hash **users);

void delete_user(struct my_hash *user, struct my_hash *users);

void delete_all(struct my_hash *users);

void print_users(struct my_hash *users);

int by_id(const struct my_hash *a, const struct my_hash *b);

// 用于从控制台获取参数命令，后面测试用
const char *getl(const char *prompt);

#endif