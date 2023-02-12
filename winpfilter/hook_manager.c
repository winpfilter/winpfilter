#include "hook_manager.h"

LIST_ENTRY HookLists[HOOK_LIST_COUNT];
PNDIS_RW_LOCK_EX HookListsLock[HOOK_LIST_COUNT];


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


NTSTATUS InitializeFilterHookManager(NDIS_HANDLE Handle) {
	TRACE_ENTER();
	NTSTATUS Status = STATUS_SUCCESS;
	for (ULONG i = 0; i < HOOK_LIST_COUNT; i++) {
		InitializeListHead(&HookLists[i]);
		HookListsLock[i] = NdisAllocateRWLock(Handle);
		if (HookListsLock == NULL) {
			Status = STATUS_UNSUCCESSFUL;
		}
	}
	if (!NT_SUCCESS(Status)) {
		FreeFilterHookManager();
	}
	TRACE_EXIT();
	return Status;
}

VOID FreeFilterHookManager() {
	TRACE_ENTER();

	for (ULONG i = 0; i < HOOK_LIST_COUNT; i++) {
		UnregisterAllHooks(i);
		NdisFreeRWLock(HookListsLock[i]);
	}

	TRACE_EXIT();
}


NTSTATUS RegisterHook(HOOK_FUNCTION HookFunction, ULONG Priority, FILTER_POINT FilterPoint) {

	PHOOK_ENTRY Entry;
	PLIST_ENTRY InsertPos;
	PHOOK_ENTRY CurrentEntry;
	LOCK_STATE_EX LockState;

	Entry = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(HOOK_ENTRY), HOOK_ENTRY_ALLOC_TAG);
	if (Entry == NULL) {
		return STATUS_UNSUCCESSFUL;
	}
	RtlZeroMemory(Entry, sizeof(HOOK_ENTRY));
	Entry->FilterPoint = FilterPoint;
	Entry->HookFunction = HookFunction;
	Entry->Priority = Priority;

	NdisAcquireRWLockWrite(HookListsLock[FilterPoint],&LockState,0);

	InsertPos = HookLists[FilterPoint].Flink;
	for (; 
		InsertPos != &HookLists[FilterPoint];
		InsertPos = InsertPos->Flink) {

		CurrentEntry = CONTAINING_RECORD(InsertPos, HOOK_ENTRY, HookLink);
		if ((Priority == CurrentEntry->Priority)
			&& (HookFunction == CurrentEntry->HookFunction)) {
			//The same hook has already been registered, refuse it!
			//*** MUST release the lock, otherwise it will lead to a deadlock
			NdisReleaseRWLock(HookListsLock[FilterPoint], &LockState);
			return STATUS_UNSUCCESSFUL;
		}
		if (Priority > CurrentEntry->Priority) {
			break;
		}
	}

	InsertTailList(InsertPos, &Entry->HookLink);

	NdisReleaseRWLock(HookListsLock[FilterPoint], &LockState);

	return STATUS_SUCCESS;
}

BOOLEAN UnregisterHook(HOOK_FUNCTION HookFunction, ULONG Priority, FILTER_POINT FilterPoint) {

	PHOOK_ENTRY CurrentEntry;
	LOCK_STATE_EX LockState;
	BOOLEAN UnregisterFlag = FALSE;

	NdisAcquireRWLockWrite(HookListsLock[FilterPoint], &LockState, 0);

	for (PLIST_ENTRY RemovePos = HookLists[FilterPoint].Flink; 
		RemovePos != &HookLists[FilterPoint];
		RemovePos = RemovePos->Flink) {

		CurrentEntry = CONTAINING_RECORD(RemovePos, HOOK_ENTRY, HookLink);
		if ((Priority == CurrentEntry->Priority)
			&& (HookFunction == CurrentEntry->HookFunction)) {
			RemoveEntryList(RemovePos);
			UnregisterFlag = TRUE;
			ExFreePoolWithTag(CurrentEntry, HOOK_ENTRY_ALLOC_TAG);
			break;
		}
	}

	NdisReleaseRWLock(HookListsLock[FilterPoint], &LockState);
	return UnregisterFlag;
}

VOID UnregisterAllHooks(FILTER_POINT FilterPoint) {

	PLIST_ENTRY TempPos;
	PHOOK_ENTRY CurrentEntry;
	LOCK_STATE_EX LockState;

	NdisAcquireRWLockWrite(HookListsLock[FilterPoint], &LockState, 0);

	for (PLIST_ENTRY RemovePos = HookLists[FilterPoint].Flink;
		RemovePos != &HookLists[FilterPoint];
		) {

		TempPos = RemovePos;
		RemovePos = RemovePos->Flink;

		CurrentEntry = CONTAINING_RECORD(TempPos, HOOK_ENTRY, HookLink);
		RemoveEntryList(TempPos);
		ExFreePoolWithTag(CurrentEntry, HOOK_ENTRY_ALLOC_TAG);
	}

	NdisReleaseRWLock(HookListsLock[FilterPoint], &LockState);

}

HOOK_RESULT FilterEthernetPacket(BYTE* EthernetBuffer, ULONG* DataLength,ULONG BufferLength, FILTER_POINT FilterPoint, NET_LUID InterfaceLuid,UCHAR DispatchLevel) {

	LOCK_STATE_EX LockState;
	HOOK_RESULT Result;
	HOOK_ACTION Action;
	PHOOK_ENTRY CurrentEntry;
	BOOLEAN TruncateChainFlag = FALSE;

	// Default result
	Result.Accept = TRUE;
	Result.Modified = FALSE;

	NdisAcquireRWLockRead(HookListsLock[FilterPoint], &LockState, DispatchLevel);

	for (PLIST_ENTRY Hook = HookLists[FilterPoint].Flink;
		Hook != &HookLists[FilterPoint];
		Hook = Hook->Flink
		) {

		CurrentEntry = CONTAINING_RECORD(Hook, HOOK_ENTRY, HookLink);
		Action = (*CurrentEntry->HookFunction)(InterfaceLuid, FilterPoint, BufferLength, EthernetBuffer, DataLength);
		switch (Action)
		{
		case HOOK_ACTION_ACCEPT:
			Result.Accept = TRUE;
			break;
		case HOOK_ACTION_MODIFIED:
			Result.Modified = TRUE;
			break;
		case HOOK_ACTION_TRUNCATE_CHAIN:
			Result.Accept = TRUE;
			TruncateChainFlag = TRUE;
			break;
		default:
			// Drop the packet by default.
			Result.Accept = FALSE;
			Result.Modified = FALSE;
			TruncateChainFlag = TRUE;
			break;
		}
		if (*DataLength > BufferLength) {
			Result.Accept = FALSE;
			Result.Modified = FALSE;
			TruncateChainFlag = TRUE;
		}
		// Exit loop if TruncateChainFlag set 
		if (TruncateChainFlag) {
			break;
		}
	}

	NdisReleaseRWLock(HookListsLock[FilterPoint], &LockState);
	
	// When the buffer been modified, create a new NBL and then drop the old one.
	if (Result.Modified) {
		Result.Accept = FALSE;
	}

	return Result;
}