#pragma once
#include "winpfilter.h"

typedef struct _QUEUE {
	ULONG ItemCounts;
	KSEMAPHORE CountsSemaphore;
	SINGLE_LIST_ENTRY Queue;
	PSINGLE_LIST_ENTRY QueueTail;
	NDIS_SPIN_LOCK HeadLock;
	NDIS_SPIN_LOCK TailLock;
}QUEUE,*PQUEUE;