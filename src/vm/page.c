#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <list.h>

void
spage_load (struct spage_entry *spe)
{
	struct thread *curr = thread_current ();
	void *frame = palloc_get_page (PAL_USER);
	bool writable = spe->writable;

	if (frame == NULL) 
	{
		frame = evict_frame ();
		if (frame == NULL)
			return;
	}

	bool success = pagedir_set_page (curr->pagedir, spe->vaddr, frame, writable);
	
	if (success)
	{
		insert_frame (frame);
		swap_in (spe);
		hash_delete (&curr->spage_table, &spe->elem);
	}
	else palloc_free_page (frame);
}

struct spage_entry *
spage_get_entry (void *vaddr)
{
	struct thread *curr = thread_current ();
	struct spage_entry spe;

	spe.vaddr = vaddr;
	struct hash_elem *spe_elem = hash_find (&curr->spage_table, &spe.elem);

	if (spe_elem == NULL)
		return NULL;

	return hash_entry (spe_elem, struct spage_entry, elem);
}

struct spage_entry *
spage_get_entry_from_thread (void *vaddr, struct thread *t)
{
	struct spage_entry spe;

	spe.vaddr = vaddr;
	struct hash_elem *spe_elem = hash_find (&t->spage_table, &spe.elem);

	if (spe_elem == NULL)
		return NULL;

	return hash_entry (spe_elem, struct spage_entry, elem);
}

void
spage_insert_upage (uint8_t *upage)
{
	struct spage_entry *spe;
	spe = malloc (sizeof *spe);

	spe->vaddr = upage;
	// spe->loaded = false;
	// spe->swapped = false;

	spage_insert_entry (spe);
}

struct hash_elem *
spage_insert_entry (struct spage_entry *spe)
{
	struct thread *curr = thread_current ();

	return hash_insert (&curr->spage_table, &spe->elem);
}

bool
hash_comp_func (const struct hash_elem *x,
    const struct hash_elem *y, void *aux UNUSED)
{
 	return hash_entry (x, struct spage_entry, elem)->vaddr 
 	< hash_entry (y, struct spage_entry, elem)->vaddr;
}

unsigned
hash_calc_func (const struct hash_elem *e, void *aux UNUSED)
{
	struct spage_entry *spe = hash_entry (e, struct spage_entry, elem);
	return hash_bytes (&spe->vaddr, sizeof spe->vaddr);
}

void
hash_free_func (const struct hash_elem *e, void *aux UNUSED)
{
	struct spage_entry *spe = hash_entry (e, struct spage_entry, elem);

	if (spe == NULL) return;

	//조건문에 넣어야 하려나?
	bitmap_flip (swap_bitmap, spe->index);
	free (spe);
	return;
}

void
stack_growth (void *addr)
{
	void *frame = palloc_get_page (PAL_USER | PAL_ZERO);
	struct thread *curr = thread_current ();
	bool success = false;

	if (frame)
		success = pagedir_set_page (curr->pagedir, pg_round_down (addr), frame, true);
	if (!success)
		free_frame (frame);
}


