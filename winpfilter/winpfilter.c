/*
 * File Name:		winptables.c
 * Description:		The entry point and driver dispatch subroutines for the winpfilter NDIS Driver
 * Author:			HBSnail
 */

#include "winpfilter.h"
#include "filter_subroutines.h"
#include "route.h"
#include "hook_manager.h"
#include "global_variables.h"
#include "communication.h"


VOID DriverUnload(PDRIVER_OBJECT DriverObject) {

	UNICODE_STRING R0HookDeviceLink = (UNICODE_STRING)RTL_CONSTANT_STRING(WINPFILTER_R0HOOK_COMMUNICATION_DEVICE_LINK);
	UNICODE_STRING R3CmdDeviceLink = (UNICODE_STRING)RTL_CONSTANT_STRING(WINPFILTER_R3COMMAND_COMMUNICATION_DEVICE_LINK);

	TRACE_ENTER();

	IoDeleteSymbolicLink(&R3CmdDeviceLink);
	if (WPFilterR3CommandCommunicationDevice != NULL) {
		IoDeleteDevice(WPFilterR3CommandCommunicationDevice);
	}
	IoDeleteSymbolicLink(&R0HookDeviceLink);
	if (WPFilterR0HookCommunicationDevice != NULL) {
		IoDeleteDevice(WPFilterR0HookCommunicationDevice);
	}
	FreeFilterHookManager();
	StopMonitorUnicastIpChange();
	StopMonitorSystemRouteTableChange();
	NdisFDeregisterFilterDriver(FilterDriverHandle);
	FreeInterfaceIPCache();
	NdisFreeSpinLock(&FilterListLock);

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
	UNICODE_STRING R0HookDeviceName = (UNICODE_STRING)RTL_CONSTANT_STRING(WINPFILTER_R0HOOK_COMMUNICATION_DEVICE_NAME);
	UNICODE_STRING R0HookDeviceLink = (UNICODE_STRING)RTL_CONSTANT_STRING(WINPFILTER_R0HOOK_COMMUNICATION_DEVICE_LINK);
	UNICODE_STRING R3CmdDeviceName = (UNICODE_STRING)RTL_CONSTANT_STRING(WINPFILTER_R3COMMAND_COMMUNICATION_DEVICE_NAME);
	UNICODE_STRING R3CmdDeviceLink = (UNICODE_STRING)RTL_CONSTANT_STRING(WINPFILTER_R3COMMAND_COMMUNICATION_DEVICE_LINK);

	// See https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/sddl-for-device-objects for more details about SDDL
#define SDDL_DEVOBJ_KERNEL_ONLY L"D:P"
#define SDDL_DEVOBJ_SYS_RWX_ADM_RWX L"D:P(A;;GRGW;;;SY)(A;;GRGW;;;BA)"
	UNICODE_STRING SDDLKernelOnly = (UNICODE_STRING)RTL_CONSTANT_STRING(SDDL_DEVOBJ_KERNEL_ONLY);
	UNICODE_STRING SDDLR3Addministrator = (UNICODE_STRING)RTL_CONSTANT_STRING(SDDL_DEVOBJ_SYS_RWX_ADM_RWX);
	const GUID WPFilterR0HookDeviceGUID = { 0xDABDE8BDL, 0xF189, 0x4970, { 0x95, 0x10, 0xD8, 0x6E, 0xF6, 0xB1, 0xAF, 0x0A } };
	const GUID WPFilterR3CmdDeviceGUID = { 0x4E3304F1L, 0x774E, 0x4AA3, { 0x87,0x57, 0xD1, 0xE4, 0x25, 0x1E, 0xC9, 0x42 } };

	TRACE_ENTER();

	do
	{	//Check the NDIS version
		//winpfilter only support NDIS_VERSION >= 6.20(win7&server2008)
		ULONG ndisVersion = NdisGetVersion();
		if (ndisVersion < NDIS_RUNTIME_VERSION_620)
		{
			Status = NDIS_STATUS_UNSUPPORTED_REVISION;
			break;
		}

		//Init driver dispatch subroutine
		DriverObject->DriverUnload = DriverUnload;

		//Prepare the variables used in NDIS filter driver registration
		InitWPFilter(&FChars);

		//Init the write lock for filterModuleList and link list head
		NdisAllocateSpinLock(&FilterListLock);
		InitializeListHead(&FilterModuleList);


		Status = InitializeInterfaceIPCache(FilterDriverObject);
		if (!NT_SUCCESS(Status)) {
			break;
		}
		//Register the NDIS filter driver
		FilterDriverHandle = NULL;
		FilterDriverObject = DriverObject;

		Status = NdisFRegisterFilterDriver(DriverObject, 
			FilterDriverObject, &FChars, &FilterDriverHandle);
		//Check if the NDIS filter driver registered successful
		if (!NT_SUCCESS(Status)) {
			break;
		}

		//Monitor route table change
		Status = StartMonitorSystemRouteTableChange();
		if (!NT_SUCCESS(Status)) {
			break;
		}

		Status = StartMonitorUnicastIpChange();
		if (!NT_SUCCESS(Status)) {
			break;
		}

		Status = InitializeFilterHookManager(FilterDriverObject);
		if (!NT_SUCCESS(Status)) {
			break;
		}

		//Create the device to communicate with Ring0 Hook registration
		Status = IoCreateDeviceSecure(DriverObject, 0, &R0HookDeviceName,
			FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE,
			&SDDLKernelOnly, (LPCGUID)&WPFilterR0HookDeviceGUID, &WPFilterR0HookCommunicationDevice);
		if (!NT_SUCCESS(Status)) {
			break;
		}
		//Create a symbolic link for the device
		Status = IoCreateSymbolicLink(&R0HookDeviceLink, &R0HookDeviceName);
		if (!NT_SUCCESS(Status)) {
			break;
		}

		//Create the device to communicate with Ring3 command
		Status = IoCreateDeviceSecure(DriverObject, 0, &R3CmdDeviceName,
			FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE,
			&SDDLR3Addministrator, (LPCGUID)&WPFilterR3CmdDeviceGUID, &WPFilterR3CommandCommunicationDevice);
		if (!NT_SUCCESS(Status)) {
			break;
		}
		//Create a symbolic link for the device
		Status = IoCreateSymbolicLink(&R3CmdDeviceLink, &R3CmdDeviceName);
		if (!NT_SUCCESS(Status)) {
			break;
		}

		//Set the IRP dispatch subroutines for driver
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WPFilterCommDeviceIOCtl;
		DriverObject->MajorFunction[IRP_MJ_READ] = WPFilterCommDeviceRead;
		DriverObject->MajorFunction[IRP_MJ_WRITE] = WPFilterCommDeviceWrite;


	} while (FALSE);

	if (!NT_SUCCESS(Status)) {

		IoDeleteSymbolicLink(&R3CmdDeviceLink);
		if (WPFilterR3CommandCommunicationDevice != NULL) {
			IoDeleteDevice(WPFilterR3CommandCommunicationDevice);
		}
		IoDeleteSymbolicLink(&R0HookDeviceLink);
		if (WPFilterR0HookCommunicationDevice != NULL) {
			IoDeleteDevice(WPFilterR0HookCommunicationDevice);
		}
		FreeFilterHookManager();
		StopMonitorUnicastIpChange();
		StopMonitorSystemRouteTableChange();
		NdisFDeregisterFilterDriver(FilterDriverHandle);
		FreeInterfaceIPCache();
		NdisFreeSpinLock(&FilterListLock);
	}

	TRACE_EXIT();
	TRACE_DBG("%X\n", Status);
	return Status;
}
