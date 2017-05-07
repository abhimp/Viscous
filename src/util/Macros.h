/*
 * Macros.h
 *
 *  Created on: 29-Mar-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_MACROS_H_
#define SRC_UTIL_MACROS_H_

#define APP_LL_DEFINE(x_) x_ *next, *prev;
#define APP_LL_INIT_LIST next(NULL), prev(NULL)
#define APP_LL_RESET {next = NULL; prev = NULL;}
#define APP_LL_RESET_PTR(node) {node->next = NULL; node->prev = NULL;}

#define APP_LL_ADD_NEXT(node1, node2) \\
        { \\
            auto x = node1->next; \\
            node1->next = node2; \\
            node2->next = x; \\
        }

#define APP_LL_ADD_PREV(node1, node2) \\
        { \\
            auto x = node1->prev; \\
            node1->prev = node2; \\
            node2->prev = x; \\
        }

#define APP_LL_DEL_NEXT(node) node->next = node->next ? node->next->next : node->next

#define APP_LL_QUEUE_DEFINE(type_) type_ *type_##beg, *type_##end; int type_##QueueSize; std::mutex type_##QueueLock;
#define APP_LL_QUEUE_INIT_LIST(type_) type_##beg(NULL), type_##end(NULL), type_##QueueSize(0)
#define APP_LL_QUEUE_RESET(type_) type_##beg = NULL; type_##end = NULL; type_##QueueSize = 0;

#define APP_LL_QUEUE_ADD_FUNC(type_) void addToQueue##type_(type_ *node) { \
    type_##QueueLock.lock(); \
    node->next = NULL; \
    if(type_##beg != NULL){ \
        type_##end->next = node; \
        type_##end = node; \
    } \
    else{ \
        type_##beg = type_##end = node; \
    } \
    type_##QueueSize ++; \
    type_##QueueLock.unlock(); \
}

#define APP_LL_QUEUE_REMOVE_FUNC(type_) type_ *getFromQueue##type_() { \
    type_##QueueLock.lock(); \
    if (type_##beg == NULL){ \
        type_##QueueLock.unlock(); \
        return NULL; \
    } \
    auto tmp = type_##beg; \
    type_##beg = type_##beg->next; \
    if(type_##beg == NULL){ \
        type_##end = NULL; \
    } \
    type_##QueueSize --; \
    type_##QueueLock.unlock(); \
    tmp->next = NULL;\
    return tmp; \
}

#endif /* SRC_UTIL_MACROS_H_ */
