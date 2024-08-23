#pragma once
#include "winpfilter.h"

typedef struct _QUEUE {
	ULONG ItemCounts;
	KEVENT DataEnqueueEvent;
	LIST_ENTRY QueueLink;
	NDIS_SPIN_LOCK QueueLock;
}QUEUE,*PQUEUE;

VOID InitializeQueue(PQUEUE Queue);
VOID FreeQueue(PQUEUE Queue);
PLIST_ENTRY Dequeue(PQUEUE Queue);
VOID Enqueue(PQUEUE Queue, PLIST_ENTRY Item);