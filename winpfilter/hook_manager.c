#include "hook_manager.h"

LIST_ENTRY HookLists[HOOK_LIST_COUNT];
NDIS_SPIN_LOCK HookListsLock[HOOK_LIST_COUNT];


//Debug functions
#ifdef DBG

VOID PrintHookList(PLIST_ENTRY Head) {

	PHOOK_ENTRY CurrentEntry;

	for (PLIST_ENTRY i = Head->Flink; i != Head; i = i->Flink) {
		CurrentEntry = CONTAINING_RECORD(i, HOOK_ENTRY, HookLink);
		TRACE_DBG("FilterPoint: %d\nPriority: %d\nHookAddr: %p\n",
			CurrentEntry->FilterPoint, 
			CurrentEntry->Priority, 
			CurrentEntry->HookFunction);
	}
}

VOID PrintHookTable() {
	for (ULONG i = 0; i < HOOK_LIST_COUNT; i++) {
		TRACE_DBG("HOOK LIST ID: %d\n", i);
		PrintHookList(&HookLists[i]);
	}
}

#endif


VOID InitializeFilterHookManager() {
	TRACE_ENTER();

	for (ULONG i = 0; i < HOOK_LIST_COUNT; i++) {
		InitializeListHead(&HookLists[i]);
		NdisAllocateSpinLock(&HookListsLock[i]);
	}

	TRACE_EXIT();
}

VOID FreeFilterHookManager() {
	TRACE_ENTER();

	for (ULONG i = 0; i < HOOK_LIST_COUNT; i++) {
		UnregisterAllHooks(i);
		NdisFreeSpinLock(&HookListsLock[i]);
	}

	TRACE_EXIT();
}


NTSTATUS RegisterHook(HOOK_FUNCTION HookFunction, ULONG Priority, ULONG FilterPoint) {

	PHOOK_ENTRY Entry;
	PLIST_ENTRY InsertPos;
	PHOOK_ENTRY CurrentEntry;

	Entry = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(HOOK_ENTRY), HOOK_ENTRY_ALLOC_TAG);
	if (Entry == NULL) {
		return STATUS_UNSUCCESSFUL;
	}
	RtlZeroMemory(Entry, sizeof(HOOK_ENTRY));
	Entry->FilterPoint = FilterPoint;
	Entry->HookFunction = HookFunction;
	Entry->Priority = Priority;

	NdisAcquireSpinLock(&HookListsLock[FilterPoint]);

	InsertPos = HookLists[FilterPoint].Flink;
	for (; 
		InsertPos != &HookLists[FilterPoint];
		InsertPos = InsertPos->Flink) {

		CurrentEntry = CONTAINING_RECORD(InsertPos, HOOK_ENTRY, HookLink);
		if ((Priority == CurrentEntry->Priority)
			&& (HookFunction == CurrentEntry->HookFunction)) {
			//The same hook has already been registered, refuse it!
			//*** MUST release the spin lock, otherwise it will lead to a deadlock
			NdisReleaseSpinLock(&HookListsLock[FilterPoint]);
			return STATUS_UNSUCCESSFUL;
		}
		if (Priority > CurrentEntry->Priority) {
			break;
		}
	}

	InsertTailList(InsertPos, Entry);

	NdisReleaseSpinLock(&HookListsLock[FilterPoint]);

	return STATUS_SUCCESS;
}

VOID UnregisterHook(HOOK_FUNCTION HookFunction, ULONG Priority, ULONG FilterPoint) {

	PHOOK_ENTRY CurrentEntry;

	NdisAcquireSpinLock(&HookListsLock[FilterPoint]);

	for (PLIST_ENTRY RemovePos = HookLists[FilterPoint].Flink; 
		RemovePos != &HookLists[FilterPoint];
		RemovePos = RemovePos->Flink) {

		CurrentEntry = CONTAINING_RECORD(RemovePos, HOOK_ENTRY, HookLink);
		if ((Priority == CurrentEntry->Priority)
			&& (HookFunction == CurrentEntry->HookFunction)) {
			RemoveEntryList(RemovePos);
			ExFreePoolWithTag(CurrentEntry, HOOK_ENTRY_ALLOC_TAG);
			break;
		}
	}

	NdisReleaseSpinLock(&HookListsLock[FilterPoint]);

}

VOID UnregisterAllHooks(ULONG FilterPoint) {

	PLIST_ENTRY TempPos;
	PHOOK_ENTRY CurrentEntry;

	NdisAcquireSpinLock(&HookListsLock[FilterPoint]);

	for (PLIST_ENTRY RemovePos = HookLists[FilterPoint].Flink;
		RemovePos != &HookLists[FilterPoint];
		) {

		TempPos = RemovePos;
		RemovePos = RemovePos->Flink;

		CurrentEntry = CONTAINING_RECORD(TempPos, HOOK_ENTRY, HookLink);
		RemoveEntryList(TempPos);
		ExFreePoolWithTag(CurrentEntry, HOOK_ENTRY_ALLOC_TAG);
	}

	NdisReleaseSpinLock(&HookListsLock[FilterPoint]);

}