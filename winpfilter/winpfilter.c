/*
 * File Name:		winptables.c
 * Description:		The entry point and driver dispatch subroutines for the winpfilter NDIS Driver
 * Author:			HBSnail
 */

#include "winpfilter.h"
#include "filter_subroutines.h"
#include "route.h"
#include "hook_manager.h"

NDIS_HANDLE FilterDriverHandle = NULL;
NDIS_HANDLE FilterDriverObject = NULL;


NDIS_SPIN_LOCK FilterListLock;
LIST_ENTRY FilterModuleList;

DEVICE_OBJECT* WPFilterCommunicationDevice;


VOID DriverUnload(PDRIVER_OBJECT driverObject) {
	UNICODE_STRING DeviceLink = (UNICODE_STRING)RTL_CONSTANT_STRING(WINPFILTER_COMMUNICATION_DEVICE_LINK);

	TRACE_ENTER();

	NdisFreeSpinLock(&FilterListLock);
	NdisFDeregisterFilterDriver(FilterDriverHandle);
	IoDeleteSymbolicLink(&DeviceLink);
	IoDeleteDevice(WPFilterCommunicationDevice);
	StopMonitorSystemRouteTableChange();
	FreeFilterHookManager();

	TRACE_EXIT();
}


VOID InitWPFilter(PNDIS_FILTER_DRIVER_CHARACTERISTICS pFChars) {

	NDIS_STRING FriendlyName = (UNICODE_STRING)RTL_CONSTANT_STRING(FILTER_FRIENDLY_NAME);
	NDIS_STRING UniqueName = (UNICODE_STRING)RTL_CONSTANT_STRING(FILTER_GUID);
	NDIS_STRING ServiceName = (UNICODE_STRING)RTL_CONSTANT_STRING(FILTER_SERVICE_NAME);

	NdisZeroMemory(pFChars, sizeof(NDIS_FILTER_DRIVER_CHARACTERISTICS));

	pFChars->Header.Type = NDIS_OBJECT_TYPE_FILTER_DRIVER_CHARACTERISTICS;
	pFChars->Header.Size = sizeof(NDIS_FILTER_DRIVER_CHARACTERISTICS);
	pFChars->Header.Revision = NDIS_FILTER_CHARACTERISTICS_REVISION_2;

	pFChars->MajorNdisVersion = NDIS_MAJOR_VERSION;
	pFChars->MinorNdisVersion = NDIS_MINOR_VERSION;
	pFChars->MajorDriverVersion = 1;
	pFChars->MinorDriverVersion = 0;
	pFChars->Flags = 0;

	pFChars->FriendlyName = FriendlyName;
	pFChars->UniqueName = UniqueName;
	pFChars->ServiceName = ServiceName;

	pFChars->SetOptionsHandler = WPFilterSetOptions;
	pFChars->AttachHandler = WPFilterAttach;
	pFChars->DetachHandler = WPFilterDetach;
	pFChars->RestartHandler = WPFilterRestart;
	pFChars->PauseHandler = WPFilterPause;
	pFChars->SetFilterModuleOptionsHandler = WPFilterSetModuleOptions;
	pFChars->ReceiveNetBufferListsHandler = WPFilterReceiveFromNIC; //When NIC receive data from the wire this subroutine will be invoked
	pFChars->ReturnNetBufferListsHandler = WPFilterSendToUpperFinished; //When the data from NIC to the upper finished processing by the upper NDIS protocol driver this subroutine will be invoked
	pFChars->SendNetBufferListsHandler = WPFilterReceiveFromUpper; //When upper NDIS protocol driver wants to send packet this subroutine will be invoked
	pFChars->SendNetBufferListsCompleteHandler = WPFilterSendToNICFinished;//When the data upper NDIS protocol driver send finished processing by the NIC this subroutine will be invoked

	//Set other unused dispatch subroutines to NULL
	pFChars->OidRequestHandler = NULL;
	pFChars->OidRequestCompleteHandler = NULL;
	pFChars->CancelOidRequestHandler = NULL;
	pFChars->DevicePnPEventNotifyHandler = NULL;
	pFChars->NetPnPEventHandler = NULL;
	pFChars->StatusHandler = NULL;
	pFChars->CancelSendNetBufferListsHandler = NULL;


}



NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {

	NDIS_FILTER_DRIVER_CHARACTERISTICS FChars;
	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
	UNICODE_STRING DeviceName = (UNICODE_STRING)RTL_CONSTANT_STRING(WINPFILTER_COMMUNICATION_DEVICE_NAME);
	UNICODE_STRING DeviceLink = (UNICODE_STRING)RTL_CONSTANT_STRING(WINPFILTER_COMMUNICATION_DEVICE_LINK);

	// See https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/sddl-for-device-objects for more details about SDDL
#define SDDL_DEVOBJ_KERNEL_ONLY L"D:P"
	UNICODE_STRING SDDLKernelOnly = (UNICODE_STRING)RTL_CONSTANT_STRING(SDDL_DEVOBJ_KERNEL_ONLY);
	const GUID WPFilterDeviceGUID = { 0xE2A61EE9L, 0x3A05, 0xA07D, { 0x31, 0x2E, 0x7B, 0x3A, 0xD0, 0x79, 0x18, 0x8B } };

	TRACE_ENTER();

	do
	{	//Check the NDIS version
		//winptables only support NDIS_VERSION >= 6.20(win7&server2008)
		ULONG ndisVersion = NdisGetVersion();
		if (ndisVersion < NDIS_RUNTIME_VERSION_620)
		{
			Status = NDIS_STATUS_UNSUPPORTED_REVISION;
			break;
		}

		//Prepare the variables used in NDIS filter driver registration
		InitWPFilter(&FChars);

		//Init the write lock for filterModuleList and link list head
		NdisAllocateSpinLock(&FilterListLock);
		InitializeListHead(&FilterModuleList);

		//Register the NDIS filter driver
		FilterDriverHandle = NULL;
		FilterDriverObject = DriverObject;
		Status = NdisFRegisterFilterDriver(DriverObject, 
			FilterDriverObject, &FChars, &FilterDriverHandle);

		//Check if the NDIS filter driver registered successful
		if (!NT_SUCCESS(Status)) {
			NdisFreeSpinLock(&FilterListLock);
			break;
		}

		//Init driver dispatch subroutine
		DriverObject->DriverUnload = DriverUnload;

		//Create the device to communicate with Ring3 
		Status = IoCreateDeviceSecure(DriverObject, 0, &DeviceName, 
			FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, 
			&SDDLKernelOnly, (LPCGUID)&WPFilterDeviceGUID, &WPFilterCommunicationDevice);

		if (!NT_SUCCESS(Status)) {
			NdisFreeSpinLock(&FilterListLock);
			NdisFDeregisterFilterDriver(FilterDriverHandle);
			break;
		}

		//Create a symbolic link for the device
		Status = IoCreateSymbolicLink(&DeviceLink, &DeviceName);
		if (!NT_SUCCESS(Status)) {
			NdisFreeSpinLock(&FilterListLock);
			NdisFDeregisterFilterDriver(FilterDriverHandle);
			IoDeleteDevice(WPFilterCommunicationDevice);
			break;
		}


		//Monitor route table change
		//WPFilterRouteTableChangeNotifyHandle = &row;
		Status = StartMonitorSystemRouteTableChange();

		if (!NT_SUCCESS(Status)) {
			NdisFreeSpinLock(&FilterListLock);
			NdisFDeregisterFilterDriver(FilterDriverHandle);
			IoDeleteDevice(WPFilterCommunicationDevice);
			IoDeleteSymbolicLink(&DeviceLink);
			break;
		}

		InitializeFilterHookManager();


		//Set the IRP dispatch subroutines for driver
		/*driverObject->MajorFunction[IRP_MJ_CREATE] = WPTCommDeviceCreate;
		driverObject->MajorFunction[IRP_MJ_CLOSE] = WPTCommDeviceClose;
		driverObject->MajorFunction[IRP_MJ_CLEANUP] = WPTCommDeviceClean;
		driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WPTCommDeviceIOCtl;
		driverObject->MajorFunction[IRP_MJ_READ] = WPTCommDeviceRead;
		driverObject->MajorFunction[IRP_MJ_WRITE] = WPTCommDeviceWrite;*/

	} while (FALSE);



	TRACE_EXIT();
	TRACE_DBG("%X\n", Status);
	return Status;
}
