#include "communication.h"
#include "global_variables.h"
#include "hook_manager.h"

NTSTATUS WPFilterCommDeviceIOCtl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	TRACE_ENTER();
	NTSTATUS Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	PIO_STACK_LOCATION StackLocation = IoGetCurrentIrpStackLocation(Irp);

	ULONG CTLCode = StackLocation->Parameters.DeviceIoControl.IoControlCode;
	ULONG InputBufferLeng = StackLocation->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputBufferLeng = StackLocation->Parameters.DeviceIoControl.OutputBufferLength;
	BYTE* KernelBuffer = Irp->AssociatedIrp.SystemBuffer;
	ULONG Function = (CTLCode & 0x3FFC) >> 2;
	do
	{
		if (KernelBuffer == NULL) {
			// Error
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		if (DeviceObject == WPFilterR0HookCommunicationDevice) {
			// Used for hook management
			PWINPFILTER_R0_HOOK_OP_STRUCTURE WinpfilterR0HookOPData = (PWINPFILTER_R0_HOOK_OP_STRUCTURE)KernelBuffer;

			// Input:
			//  ULONG Mode :always 0 
			//  HOOK_FUNCTION (PVOID) hook_function
			//  ULONG priority
			//  FILTER_POINT filter_point
			// Output
			//  ULONG status : 1 - success ; 0 - unsuccess
			//  HOOK_FUNCTION (PVOID) hook_function
			//  ULONG priority
			//  FILTER_POINT filter_point

			if ((InputBufferLeng < sizeof(PWINPFILTER_R0_HOOK_OP_STRUCTURE)) || (OutputBufferLeng < sizeof(PWINPFILTER_R0_HOOK_OP_STRUCTURE))) {
				Status = STATUS_UNSUCCESSFUL;
				break;
			}
			if (WinpfilterR0HookOPData->FilterPoint >= HOOK_LIST_COUNT || WinpfilterR0HookOPData->Mode != 0) {
				Status = STATUS_UNSUCCESSFUL; 
				break;
			}

			Irp->IoStatus.Information = sizeof(PWINPFILTER_R0_HOOK_OP_STRUCTURE);

			switch (CTLCode)
			{
			case WINPFILTER_CTL_CODE_REGISTER_HOOK:
				Status = RegisterHook((HOOK_FUNCTION)WinpfilterR0HookOPData->HookFunction, WinpfilterR0HookOPData->Priority, WinpfilterR0HookOPData->FilterPoint);
				if (!NT_SUCCESS(Status)) {
					break;
				}
				WinpfilterR0HookOPData->Mode = TRUE;
				break;
			case WINPFILTER_CTL_CODE_UNREGISTER_HOOK:
				WinpfilterR0HookOPData->Mode = UnregisterHook((HOOK_FUNCTION)WinpfilterR0HookOPData->HookFunction, WinpfilterR0HookOPData->Priority, WinpfilterR0HookOPData->FilterPoint);
				break;
			default:
				Status = STATUS_UNSUCCESSFUL;
				break;
			}
			
			if (!NT_SUCCESS(Status)) {
				RtlZeroMemory(KernelBuffer,OutputBufferLeng);
				Irp->IoStatus.Information = 0;
			}

		}
		else if (DeviceObject == WPFilterR3CommandCommunicationDevice) {
			// Used for command

			PWINPFILTER_R3_HOOK_OP_STRUCTURE WinpfilterR3HookOPData = (PWINPFILTER_R3_HOOK_OP_STRUCTURE)KernelBuffer;

			// Input:
			//  ULONG [0] mode : 1 set  other get
			//  ULONG [1] value
			// Output
			//  ULONG [0] function
			//  ULONG [1] value

			if ((InputBufferLeng < sizeof(PWINPFILTER_R3_HOOK_OP_STRUCTURE)) || (OutputBufferLeng < sizeof(PWINPFILTER_R3_HOOK_OP_STRUCTURE))) {
				Status = STATUS_UNSUCCESSFUL;
				break;
			}

			Irp->IoStatus.Information = sizeof(PWINPFILTER_R3_HOOK_OP_STRUCTURE);
			switch (CTLCode)
			{
			case WINPFILTER_CTL_CODE_VERSION:
				// Get only: Version
				WinpfilterR3HookOPData->Value = WINPFILTER_VERSION;
				break;
			case WINPFILTER_CTL_CODE_FORWARDING_MODE:
				if (WinpfilterR3HookOPData->Mode == 1 && WinpfilterR3HookOPData->Value <= 2) {
					// Set forwarding mode
					//  IP_FORWARDING_MODE_SYSTEM		0
					//  IP_FORWARDING_MODE_WINPFILTER	1
					//  IP_FORWARDING_MODE_DISABLE		2
					IPForwardingMode = (BYTE)WinpfilterR3HookOPData->Value;
				}
				WinpfilterR3HookOPData->Value = IPForwardingMode;
				break;
			case WINPFILTER_CTL_CODE_RX_BAD_IP_CSUM:
				if (WinpfilterR3HookOPData->Mode == 1 && WinpfilterR3HookOPData->Value <= 1) {
					// Set IndicateModifiedRxPacketWithBadIPChecksumAsGood
					//  FALSE	0
					//  TRUE	1
					IndicateModifiedRxPacketWithBadIPChecksumAsGood = (BYTE)WinpfilterR3HookOPData->Value;
				}
				WinpfilterR3HookOPData->Value = IndicateModifiedRxPacketWithBadIPChecksumAsGood;
				break;
			case WINPFILTER_CTL_CODE_RX_BAD_TCP_CSUM:
				if (WinpfilterR3HookOPData->Mode == 1 && WinpfilterR3HookOPData->Value <= 1) {
					// Set IndicateModifiedRxPacketWithBadTCPChecksumAsGood
					//  FALSE	0
					//  TRUE	1
					IndicateModifiedRxPacketWithBadTCPChecksumAsGood = (BYTE)WinpfilterR3HookOPData->Value;
				}
				WinpfilterR3HookOPData->Value = IndicateModifiedRxPacketWithBadTCPChecksumAsGood;
				break;
			case WINPFILTER_CTL_CODE_RX_BAD_UDP_CSUM:
				if (WinpfilterR3HookOPData->Mode == 1 && WinpfilterR3HookOPData->Value <= 1) {
					// Set IndicateModifiedRxPacketWithBadUDPChecksumAsGood
					//  FALSE	0
					//  TRUE	1
					IndicateModifiedRxPacketWithBadUDPChecksumAsGood = (BYTE)WinpfilterR3HookOPData->Value;
				}
				WinpfilterR3HookOPData->Value = IndicateModifiedRxPacketWithBadUDPChecksumAsGood;
				break;
			case WINPFILTER_CTL_CODE_TX_NIC_IP_CSUM:
				if (WinpfilterR3HookOPData->Mode == 1 && WinpfilterR3HookOPData->Value <= 1) {
					// Set TryCalcModifiedTxPacketIPChecksumByNIC
					//  FALSE	0
					//  TRUE	1
					TryCalcModifiedTxPacketIPChecksumByNIC = (BYTE)WinpfilterR3HookOPData->Value;
				}
				WinpfilterR3HookOPData->Value = TryCalcModifiedTxPacketIPChecksumByNIC;
				break;
			case WINPFILTER_CTL_CODE_TX_NIC_TCP_CSUM:
				if (WinpfilterR3HookOPData->Mode == 1 && WinpfilterR3HookOPData->Value <= 1) {
					// Set TryCalcModifiedTxPacketTCPChecksumByNIC
					//  FALSE	0
					//  TRUE	1
					TryCalcModifiedTxPacketTCPChecksumByNIC = (BYTE)WinpfilterR3HookOPData->Value;
				}
				WinpfilterR3HookOPData->Value = TryCalcModifiedTxPacketTCPChecksumByNIC;
				break;
			case WINPFILTER_CTL_CODE_TX_NIC_UDP_CSUM:
				if (WinpfilterR3HookOPData->Mode == 1 && WinpfilterR3HookOPData->Value <= 1) {
					// Set TryCalcModifiedTxPacketUDPChecksumByNIC
					//  FALSE	0
					//  TRUE	1
					TryCalcModifiedTxPacketUDPChecksumByNIC = (BYTE)WinpfilterR3HookOPData->Value;
				}
				WinpfilterR3HookOPData->Value = TryCalcModifiedTxPacketUDPChecksumByNIC;
				break;
			case WINPFILTER_CTL_CODE_HOOK_INFO:
				// Get only: Hook info
				if (WinpfilterR3HookOPData->Value < HOOK_LIST_COUNT) {
					WinpfilterR3HookOPData->Value = GetHookInformation(WinpfilterR3HookOPData->Value, (PHOOK_INFO)(KernelBuffer+ sizeof(WINPFILTER_R3_HOOK_OP_STRUCTURE)), OutputBufferLeng- sizeof(WINPFILTER_R3_HOOK_OP_STRUCTURE));
					Irp->IoStatus.Information = OutputBufferLeng;
				}
				else {
					WinpfilterR3HookOPData->Value = ULONG_MAX;
				}
				break;
			default:
				Status = STATUS_UNSUCCESSFUL;
				break;
			}
			WinpfilterR3HookOPData->Mode = Function;
			if (!NT_SUCCESS(Status)) {
				Irp->IoStatus.Information = 0;
			}
		}
		else {
			// Error
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

	} while (FALSE);

	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	TRACE_EXIT();
	return Status;
}

NTSTATUS WPFilterCommDeviceRead(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	TRACE_ENTER();
	NTSTATUS Status = STATUS_SUCCESS;

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	if (DeviceObject == WPFilterR0HookCommunicationDevice) {
		// Used for hook management
		Status = STATUS_UNSUCCESSFUL;
	}
	else if (DeviceObject == WPFilterR3CommandCommunicationDevice) {
		// Used for command
		Status = STATUS_UNSUCCESSFUL;
	}
	else {
		// Error
		Status = STATUS_UNSUCCESSFUL;
	}

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	TRACE_EXIT();
	return Status;
}

NTSTATUS WPFilterCommDeviceWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	TRACE_ENTER();
	NTSTATUS Status = STATUS_SUCCESS;

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	if (DeviceObject == WPFilterR0HookCommunicationDevice) {
		// Used for hook management
		Status = STATUS_UNSUCCESSFUL;
	}
	else if (DeviceObject == WPFilterR3CommandCommunicationDevice) {
		// Used for command
		Status = STATUS_UNSUCCESSFUL;
	}
	else {
		// Error
		Status = STATUS_UNSUCCESSFUL;
	}

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	TRACE_EXIT();
	return Status;
}

NTSTATUS WPFilterCommDeviceCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	TRACE_ENTER();
	NTSTATUS Status = STATUS_SUCCESS;

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	TRACE_EXIT();
	return Status;
}

NTSTATUS WPFilterCommDeviceClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	TRACE_ENTER();
	NTSTATUS Status = STATUS_SUCCESS;

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	TRACE_EXIT();
	return Status;
}

NTSTATUS WPFilterCommDeviceClean(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);

	TRACE_ENTER();
	NTSTATUS Status = STATUS_SUCCESS;

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	TRACE_EXIT();
	return Status;
}