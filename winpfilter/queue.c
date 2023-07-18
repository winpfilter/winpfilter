#include "queue.h"

VOID InitializeQueue(PQUEUE Queue) {
	if (Queue == NULL) {
		return;
	}
	Queue->Queue.Next = NULL;
	Queue->ItemCounts = 0;
	Queue->QueueTail = &Queue->Queue;
	KeInitializeSemaphore(&Queue->CountsSemaphore,0,0x7fffffff);
	NdisAllocateSpinLock(&Queue->HeadLock);
	NdisAllocateSpinLock(&Queue->TailLock);
}

PSINGLE_LIST_ENTRY Dequeue(PQUEUE Queue,BOOLEAN DispatchLevel) {
	PSINGLE_LIST_ENTRY ReturnItem;
	if (!DispatchLevel) {
		KeWaitForSingleObject(&Queue->CountsSemaphore, Executive, KernelMode, FALSE, NULL);
	}
	NdisAcquireSpinLock(&Queue->HeadLock);
	ReturnItem = Queue->Queue.Next;
	if (ReturnItem != NULL) {
		Queue->Queue.Next = ReturnItem->Next;
		ReturnItem->Next = NULL;
	}
	Queue->ItemCounts--;
	NdisReleaseSpinLock(&Queue->HeadLock);
	return ReturnItem;
}

VOID Enqueue(PQUEUE Queue,PSINGLE_LIST_ENTRY Item) {
	NdisAcquireSpinLock(&Queue->TailLock);
	Queue->QueueTail->Next = Item;
	Queue->QueueTail = Item;
	Queue->ItemCounts++;
	KeReleaseSemaphore(&Queue->CountsSemaphore, IO_NO_INCREMENT, 1, FALSE);
	NdisReleaseSpinLock(&Queue->TailLock);
}