#include "interface_ip_table.h"

NTSTATUS InitializeInterfaceIPCacheManager(NDIS_HANDLE Handle) {
	NTSTATUS Status = STATUS_SUCCESS;
	
	return Status;
}

VOID FreeInterfaceIPCacheManager() {

}

VOID InsertIntoInterfaceIPCache(NET_LUID InterfaceLuid, IP_ADDRESS Address) {

}

VOID DeleteAllInterfaceIPCache() {


}

BOOLEAN IsLocalIP(BYTE* Address, IP_PROTOCOLS EtherNetProtocol) {

}