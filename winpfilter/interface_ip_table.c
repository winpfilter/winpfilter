#include "interface_ip_table.h"

LIST_ENTRY EntryList;

NDIS_SPIN_LOCK IfIpELock;

NTSTATUS InitializeInterfaceIPCacheManager(NDIS_HANDLE Handle) {
	NTSTATUS Status = STATUS_SUCCESS;
	InitializeListHead(&EntryList);
	NdisAllocateSpinLock(&IfIpELock);
	return Status;
}

VOID FreeInterfaceIPCacheManager() {
	DeleteAllInterfaceIPCache();
}

VOID InsertIntoInterfaceIPCache(NET_LUID InterfaceLuid, IP_ADDRESS Address) {
	PINTERFACE_IP_INFO TableEntry = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(INTERFACE_IP_INFO), INTERFACE_IP_TABLE_ALLOC_TAG);
	if (TableEntry == NULL) {
		return;
	}
	TableEntry->InterfaceLuid = InterfaceLuid;
	TableEntry->IPAddress = Address;
	NdisAcquireSpinLock(&IfIpELock);
	InsertHeadList(&EntryList, &TableEntry->Link);
	NdisReleaseSpinLock(&IfIpELock);
	// Create index: Spec Address (v6/v4) -> TableEntry
	// and InterfaceLuid -> TableEntry
}

VOID DeleteAllInterfaceIPCache() {

	// Free all nodes in the entry list

	NdisAcquireSpinLock(&IfIpELock);
	PLIST_ENTRY NextEntry = EntryList.Flink;
	while (NextEntry != &EntryList) {
		PLIST_ENTRY CurrentEntry = NextEntry;
		NextEntry = CurrentEntry->Flink;
		RemoveEntryList(CurrentEntry);
		PINTERFACE_IP_INFO IfIpEntry = CONTAINING_RECORD(CurrentEntry, INTERFACE_IP_INFO, Link);
		ExFreePoolWithTag(IfIpEntry, INTERFACE_IP_TABLE_ALLOC_TAG);
	}
	NdisReleaseSpinLock(&IfIpELock);
}

BOOLEAN IsLocalIP(BYTE* Address, IP_PROTOCOLS Portocol) {

	PLIST_ENTRY CurrentEntry = EntryList.Flink;
	BOOLEAN CompareResult = FALSE;
	while (CurrentEntry != &EntryList) {
		PINTERFACE_IP_INFO IfIpEntry = CONTAINING_RECORD(CurrentEntry, INTERFACE_IP_INFO, Link);
		if (IfIpEntry->IPAddress.Family == Portocol) {
			if (Portocol == IP) {
				CompareResult = (RtlCompareMemory(Address, &IfIpEntry->IPAddress.IPAddress, IPV4_ADDRESS_BYTE_LENGTH) == IPV4_ADDRESS_BYTE_LENGTH);
			}
			else if (Portocol == IPV6) {
				CompareResult = (RtlCompareMemory(Address, &IfIpEntry->IPAddress.IPAddress, IPV6_ADDRESS_BYTE_LENGTH) == IPV6_ADDRESS_BYTE_LENGTH);
			}
		}
		if (CompareResult == TRUE) {
			break;
		}
		CurrentEntry = CurrentEntry->Flink;
	}

	return CompareResult;
}