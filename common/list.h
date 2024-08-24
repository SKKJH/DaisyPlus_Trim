#ifndef __LIST_H__
#define __LIST_H__

struct list_head
{
	struct list_head *next, *prev;
};

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); \
	(ptr)->prev = (ptr); \
} while (0)

#define list_entry(ptr, type, member) \
((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_first_entry(ptr, type, member) \
list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
list_entry((ptr)->prev, type, member)

#define list_for_each(pos, head) \
for (pos = (head)->next; pos != (head); \
	pos = pos->next)

#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); \
		pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
		for (pos = (head)->next, n = pos->next; pos != (head); \
			pos = n, n = pos->next)

// because there are no 'typeof' at the visual c, we add type
#define list_for_each_entry(type, pos, head, member)				\
			for (pos = list_entry((head)->next, type, member);		\
				&pos->member != (head);					\
				pos = list_entry(pos->member.next, type, member)	\
				)

/**
* list_for_each_entry_reverse - iterate backwards over list of given type.
* @pos:	the type * to use as a loop cursor.
* @head:	the head for your list.
* @member:	the name of the list_struct within the struct.
*/
#define list_for_each_entry_reverse(type, pos, head, member)			\
			for (pos = list_entry((head)->prev, type, member);			\
				&pos->member != (head); 								\
				pos = list_entry(pos->member.prev, type, member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define list_for_each_entry_safe(type, pos, n, head, member)			\
				for (pos = list_entry((head)->next, type, member),	\
					n = list_entry(pos->member.next, type, member); \
					 &pos->member != (head);					\
					 pos = n, n = list_entry(n->member.next, type, member))

#define list_add_head		list_add

#ifdef __cplusplus
extern "C" {
#endif

	void __list_add(struct list_head *_new, struct list_head *prev, struct list_head *next);
	void list_add(struct list_head *_new, struct list_head *head);
	void list_add_tail(struct list_head *_new, struct list_head *head);
	void __list_del(struct list_head *prev, struct list_head *next);
	void list_del(struct list_head *entry);
	void list_del_init(struct list_head *entry);
	void list_move(struct list_head *list, struct list_head *head);
	void list_move_head(struct list_head *list, struct list_head *head);
	void list_move_tail(struct list_head *list, struct list_head *head);
	int list_empty(struct list_head *head);
	void __list_splice(struct list_head *list, struct list_head *head);
	void list_splice(struct list_head *list, struct list_head *head);
	void list_splice_init(struct list_head *list, struct list_head *head);

	int list_count(struct list_head* head);

#ifdef __cplusplus
}
#endif

#endif	// end if #ifndef __LIST_H__


