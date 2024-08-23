#include "queue.h"

VOID InitializeQueue(PQUEUE Queue) {
	if (Queue == NULL) {
		return;
	}
	Queue->ItemCounts = 0;
	NdisAllocateSpinLock(&(Queue->QueueLock));
	InitializeListHead(&(Queue->QueueLink));
	KeInitializeEvent(&(Queue->DataEnqueueEvent), NotificationEvent, FALSE);
}

VOID FreeQueue(PQUEUE Queue) {
	if (Queue == NULL) {
		return;
	}
	NdisFreeSpinLock(&(Queue->QueueLock));
}

PLIST_ENTRY Dequeue(PQUEUE Queue) {
	PLIST_ENTRY ReturnItem = NULL;
	if (Queue == NULL) {
		return NULL;
	}
	NdisAcquireSpinLock(&(Queue->QueueLock));
	if (Queue->ItemCounts <= 0) {
		KeClearEvent(&(Queue->DataEnqueueEvent));
		NdisReleaseSpinLock(&(Queue->QueueLock));
		return NULL;
	}
	ReturnItem = RemoveHeadList(&(Queue->QueueLink));
	if ((--Queue->ItemCounts) == 0) {
		KeClearEvent(&(Queue->DataEnqueueEvent));
	}
	NdisReleaseSpinLock(&(Queue->QueueLock));
	return ReturnItem;
}

VOID Enqueue(PQUEUE Queue, PLIST_ENTRY Item) {
	if (Queue == NULL || Item == NULL) {
		return;
	}
	NdisAcquireSpinLock(&(Queue->QueueLock));
	InsertTailList(&(Queue->QueueLink), Item);
	Queue->ItemCounts++;
	KeSetEvent(&(Queue->DataEnqueueEvent), IO_NO_INCREMENT, FALSE);
	NdisReleaseSpinLock(&(Queue->QueueLock));
}