/**
 * @file hi_list_sdk.h
 *
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2017-2019. All rights reserved.
 * Description: 双向链表
 * Author: Hisilicon
 * Create: 2017-07-03
 */

 /**
  * @defgroup iot_list List
  * @ingroup system
  */
#ifndef __HI_LIST_SDK_H__
#define __HI_LIST_SDK_H__

#include <hi_types_base.h>

/****************************************************************************/
HI_START_HEADER

/*
 * 备注：本文件对链表接口重新命名，使用LTOS接口重新实现。
 */
typedef struct hi_list {
    struct hi_list *prev;
    struct hi_list *next;
} hi_list;

/*
功能描述：
    初始化头节点，注意此节点仅用于管理，不是用户输入的数据节点
 */
__attribute__((always_inline)) static inline hi_void hi_list_init(hi_list *list)
{
    list->next = list;
    list->prev = list;
}

/*
功能描述：
    把node插入为list的第一个节点
 */
__attribute__((always_inline)) static inline hi_void hi_list_head_insert(hi_list *node, hi_list *list)
{
    node->next = list->next;
    node->prev = list;
    list->next->prev = node;
    list->next = node;
}

/*
功能描述：
    把node插入为list的最后一个节点
 */
__attribute__((always_inline)) static inline hi_void hi_list_tail_insert(hi_list *node, hi_list *list)
{
    hi_list_head_insert(node, list->prev);
}

/*
功能描述：
    从链表中删除某个节点
 */
__attribute__((always_inline)) static inline hi_void hi_list_delete(hi_list *node)
{
    if (node->next == HI_NULL || node->prev == HI_NULL) {
        return;
    }

    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = (hi_list *)HI_NULL;
    node->prev = (hi_list *)HI_NULL;
}

/*
功能描述：
    删除链表的第一个节点，不释放相关内存
 */
__attribute__((always_inline)) static inline hi_list *hi_list_delete_head(hi_list *list)
{
    hi_list *del_node;

    del_node = list->next;
    if (del_node == list || del_node == HI_NULL) {
        return HI_NULL;
    }

    hi_list_delete(del_node);
    return del_node;
}

/*
功能描述：
    删除链表尾部节点，不释放相关内存
 */
__attribute__((always_inline)) static inline hi_list *hi_list_delete_tail(hi_list *list)
{
    hi_list *del_node;

    del_node = list->prev;
    if (del_node == list || del_node == HI_NULL) {
        return HI_NULL;
    }

    hi_list_delete(del_node);
    return del_node;
}

/*
功能描述：
    判断链表是否为空
 */
__attribute__((always_inline)) static inline hi_bool hi_is_list_empty(hi_list *list)
{
    if (list->next == HI_NULL || list->prev == HI_NULL) {
        return HI_TRUE;
    }
    return (hi_bool)(list->next == list);
}

/*
功能描述：
    去初始化链表，管理节点清空，其他成员节点首尾相连仍然是一个双向链表。
 */
__attribute__((always_inline)) static inline hi_void hi_list_del_init(hi_list *list)
{
    list->next->prev = list->prev;
    list->prev->next = list->next;

    list->next = list;
    list->prev = list;
}

/*
功能描述：
    将链表2加入链表1的尾部
 */
__attribute__((always_inline)) static inline hi_void hi_list_join_tail(hi_list *list1, hi_list *list2)
{
    list1->prev->next = list2->next;
    list2->next->prev = list1->prev;
    list2->prev->next = list1;
    list1->prev = list2->prev;
}

/*
功能描述：
    将链表2加入链表1的头部
 */
__attribute__((always_inline)) static inline hi_void hi_list_join_head(hi_list *list1, hi_list *list2)
{
    /* list2 is empty. */
    if (list2->next == list2) {
        return;
    }

    list2->prev->next = list1->next;
    list1->next->prev = list2->prev;
    list1->next = list2->next;
    list2->next->prev = list1;
}

/*
 * 功能描述：将链表2中从第一个元素到last_node元素摘出， 加入空链表1的头部
 */
__attribute__((always_inline)) static inline hi_void hi_list_remove_head(hi_list *list1, hi_list *list2,
                                                                         hi_list *last_node)
{
    /* 对list1赋值 */
    list1->next = list2->next;
    list1->prev = last_node;

    list2->next = last_node->next;
    ((hi_list *)(last_node->next))->prev = list2;

    last_node->next = list1;
    /* last_node为list2中第一个成员 */
    if (last_node->prev == list2) {
        last_node->prev = list1;
    }
}

#define hi_list_init_macro(_list_name) hi_list _list_name = { (hi_list*)&(_list_name), (hi_list*)&(_list_name) }

/* 获取第一个节点指针 */
#define hi_list_first(object) ((object)->next)

/* 获取最后一个节点指针 */
#define hi_list_last(object) ((object)->prev)

/*
    功能描述：
    获取到一个包含双链表的结构体的指针地址。
    举例：
    typedef struct
    {
        hi_u32 taskId
        ...
        HI_LIST_NODE stTimerList
    }TASK_ST
 */
#define hi_list_entry(item, type, member) \
    ((type*)((char*)(item) - hi_offset_of_member(type, member)))

/*
    功能列表：
        通过LIST遍历每一个成员节点所在结构体的指针入口地址。
 */
#define hi_list_for_each_entry(item, list, type, member)   \
    for ((item) = hi_list_entry((list)->next, type, member); \
         &(item)->member != (list);                          \
         (item) = hi_list_entry((item)->member.next, type, member))

/*
    功能列表：
        通过LIST遍历每一个成员节点所在结构体的指针入口地址，保存下一个节点的指针，避免当前节点处理完成后删除的场景。
 */
#define hi_list_for_each_entry_safe(list, item, pnext, type, member) \
    for ((item) = hi_list_entry((list)->next, type, member),           \
         (pnext) = hi_list_entry((item)->member.next, type, member);     \
         &(item)->member != (list);                                    \
         (item) = (pnext), (pnext) = hi_list_entry((item)->member.next, type, member))

#define hi_list_for_each_entry_continue_safe(pitem, list, item, pnext, type, member) \
    for ((item) = hi_list_entry((pitem)->next, type, member),                          \
         (pnext) = hi_list_entry((item)->member.next, type, member);                     \
         &((item)->member) != (list);                                                \
         (item) = (pnext), (pnext) = hi_list_entry((pnext)->member.next, type, member))

/* 双向链表操作简单实现 */
#define hi_list_head(list) \
    hi_list list = { &(list), &(list) }

#define hi_list_for_each(item, list) \
    for ((item) = (list)->next; (item) != (list); (item) = (item)->next)

#define hi_list_for_each_safe(item, pnext, list)                  \
    for ((item) = (list)->next, (pnext) = (item)->next; (item) != (list); \
         (item) = (pnext), (pnext) = (item)->next)

HI_END_HEADER
#endif  /* __HI_STDLIB_H__ */
