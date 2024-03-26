#include "hash_map.h"

void add_user(uint64_t user_id, struct my_hash **users)
{
    struct my_hash *s;

    HASH_FIND_UINT64(*users, &user_id, s); /* id already in the hash? */
    if (s == NULL)
    {
        s = (struct my_hash *)malloc(sizeof *s);
        s->id = user_id;
        HASH_ADD_UINT64(*users, id, s); /* id is the key field */
        // HASH_ADD(hh, users, id, sizeof(s->id), s);
    }
}

uint8_t find_user(uint64_t user_id, struct my_hash **users)
{
    struct my_hash *s;

    HASH_FIND_UINT64(*users, &user_id, s); /* s: output pointer */
    if (s == NULL)
    {
        return false;
    }
    return true;
}

void delete_user(struct my_hash *user, struct my_hash *users)
{
    HASH_DEL(users, user); /* user: pointer to deletee */
    free(user);
}

void delete_all(struct my_hash *users)
{
    struct my_hash *current_user;
    struct my_hash *tmp;

    HASH_ITER(hh, users, current_user, tmp)
    {
        HASH_DEL(users, current_user); /* delete it (users advances to next) */
        free(current_user);            /* free it */
    }
}

void print_users(struct my_hash *users)
{
    struct my_hash *s;

    for (s = users; s != NULL; s = (struct my_hash *)(s->hh.next))
    {
        printf("user id %d\n", s->id);
    }
}

int by_id(const struct my_hash *a, const struct my_hash *b)
{
    return (a->id - b->id);
}
