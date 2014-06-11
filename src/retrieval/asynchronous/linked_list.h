#ifndef __LINKED_LIST_H__
#define __LINKED_LIST_H__

#include <stdlib.h>

struct linked_list_node_t
{
    linked_list_node_t *next;
    linked_list_node_t *prev;
};

template <typename T, linked_list_node_t T::*list_node>
class linked_list_t
{
public:
    linked_list_t() { _head.next = _head.prev = &_head; }
    linked_list_t& operator =(const linked_list_t &) { _head.next = _head.prev = &_head; return *this; }
    bool is_empty() const { return _head.next == &_head; }
    void empty() { _head.next = _head.prev = &_head; }
    linked_list_node_t& head() { return _head; }
    T* entry(linked_list_node_t &node) const { return &node == &_head ? NULL : (T*)((char*)&node - (char*)_node_offset); }

    void add(T &node)
    {
        _head.next->prev = &(node.*list_node);
        (node.*list_node).next = _head.next;
        (node.*list_node).prev = &_head;
        _head.next = &(node.*list_node);
    }

    static void add_prev(T &node, T &cur)
    {
        (cur.*list_node).prev->next = &(node.*list_node);
        (node.*list_node).prev = (cur.*list_node).prev;
        (node.*list_node).next = &(cur.*list_node);
        (cur.*list_node).prev = &(node.*list_node);
    }

    static void add_next(T &node, T &cur)
    {
        (cur.*list_node).next->prev = &(node.*list_node);
        (node.*list_node).next = (cur.*list_node).next;
        (node.*list_node).prev = &(cur.*list_node);
        (cur.*list_node).next = &(node.*list_node);
    }

    static void del(T &node)
    {
        (node.*list_node).next->prev = (node.*list_node).prev;
        (node.*list_node).prev->next = (node.*list_node).next;
    }

    T* next(T &node) const
    {
        return (node.*list_node).next == &_head ? NULL : (T*)((char*)(node.*list_node).next - (char*)_node_offset);
    }

    T* prev(T &node) const
    {
        return (node.*list_node).prev == &_head ? NULL : (T*)((char*)(node.*list_node).prev - (char*)_node_offset);
    }

    T* next(linked_list_node_t &node) const
    {
        return node.next == &_head ? NULL : (T*)((char*)node.next - (char*)_node_offset);
    }

    T* prev(linked_list_node_t &node) const
    {
        return node.prev == &_head ? NULL : (T*)((char*)node.prev - (char*)_node_offset);
    }
#if 0

    void add(linked_list_node_t &node)
    {
        _head.next->prev = &node;
        node.next = _head.next;
        node.prev = &_head;
        _head.next = &node;
    }

    void del(linked_list_node_t &node)
    {
        node.next->prev = node.prev;
        node.prev->next = node.next;
    }
#endif

protected:
    static linked_list_node_t const * const _node_offset;
    linked_list_node_t _head;
};

template <typename T, linked_list_node_t T::*list_node>
linked_list_node_t const * const linked_list_t<T, list_node>::_node_offset = &(((T *)0)->*list_node);

#endif /* __LINKED_LIST_H__ */

