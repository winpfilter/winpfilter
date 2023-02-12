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
			ULONG* ModeOrStatus = (ULONG*)(KernelBuffer);
			HOOK_FUNCTION* HookFunction = (HOOK_FUNCTION*)(ModeOrStatus + 1);
			ULONG* Priority = (ULONG*)(HookFunction + 1);
			FILTER_POINT* FilterPoint = (FILTER_POINT*)(Priority + 1);
			ULONG  NeedBufferLength = (ULONG)(((ULONGLONG)(FilterPoint + 1)) - ((ULONGLONG)(KernelBuffer)));
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

			if ((InputBufferLeng < NeedBufferLength) || (OutputBufferLeng < NeedBufferLength)) {
				Status = STATUS_UNSUCCESSFUL;
				break;
			}
			if (*FilterPoint > 4 || *ModeOrStatus != 0) {
				Status = STATUS_UNSUCCESSFUL; 
				break;
			}

			Irp->IoStatus.Information = NeedBufferLength;

			switch (CTLCode)
			{
			case WINPFILTER_CTL_CODE_REGISTER_HOOK:
				Status = RegisterHook(*HookFunction,*Priority,*FilterPoint);
				if (!NT_SUCCESS(Status)) {
					break;
				}
				*ModeOrStatus = TRUE;
				break;
			case WINPFILTER_CTL_CODE_UNREGISTER_HOOK:
				*ModeOrStatus = UnregisterHook(*HookFunction, *Priority, *FilterPoint);
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
			ULONG* ModeOrFunction = (ULONG*)(KernelBuffer);
			ULONG* Value = (ULONG*)(ModeOrFunction + 1);
			ULONG  NeedBufferLength = (ULONG)(((ULONGLONG)(Value + 1)) - ((ULONGLONG)(KernelBuffer)));
			// Input:
			//  ULONG [0] mode : 1 set  other get
			//  ULONG [1] value
			// Output
			//  ULONG [0] function
			//  ULONG [1] value

			if ((InputBufferLeng < NeedBufferLength) || (OutputBufferLeng < NeedBufferLength)) {
				Status = STATUS_UNSUCCESSFUL;
				break;
			}

			Irp->IoStatus.Information = NeedBufferLength;
			switch (CTLCode)
			{
			case WINPFILTER_CTL_CODE_VERSION:
				// Get only: Version
				*Value = WINPFILTER_VERSION;
				break;
			case WINPFILTER_CTL_CODE_FORWARDING_MODE:
				if (*ModeOrFunction == 1 && *Value <= 2) {
					// Set forwarding mode
					//  IP_FORWARDING_MODE_SYSTEM		0
					//  IP_FORWARDING_MODE_WINPFILTER	1
					//  IP_FORWARDING_MODE_DISABLE		2
					IPForwardingMode = (BYTE)*Value;
				}
				*Value = IPForwardingMode;
				break;
			case WINPFILTER_CTL_CODE_RX_BAD_IP_CSUM:
				if (*ModeOrFunction == 1 && *Value <= 1) {
					// Set IndicateModifiedRxPacketWithBadIPChecksumAsGood
					//  FALSE	0
					//  TRUE	1
					IndicateModifiedRxPacketWithBadIPChecksumAsGood = (BYTE)*Value;
				}
				*Value = IndicateModifiedRxPacketWithBadIPChecksumAsGood;
				break;
			case WINPFILTER_CTL_CODE_RX_BAD_TCP_CSUM:
				if (*ModeOrFunction == 1 && *Value <= 1) {
					// Set IndicateModifiedRxPacketWithBadTCPChecksumAsGood
					//  FALSE	0
					//  TRUE	1
					IndicateModifiedRxPacketWithBadTCPChecksumAsGood = (BYTE)*Value;
				}
				*Value = IndicateModifiedRxPacketWithBadTCPChecksumAsGood;
				break;
			case WINPFILTER_CTL_CODE_RX_BAD_UDP_CSUM:
				if (*ModeOrFunction == 1 && *Value <= 1) {
					// Set IndicateModifiedRxPacketWithBadUDPChecksumAsGood
					//  FALSE	0
					//  TRUE	1
					IndicateModifiedRxPacketWithBadUDPChecksumAsGood = (BYTE)*Value;
				}
				*Value = IndicateModifiedRxPacketWithBadUDPChecksumAsGood;
				break;
			case WINPFILTER_CTL_CODE_TX_NIC_IP_CSUM:
				if (*ModeOrFunction == 1 && *Value <= 1) {
					// Set TryCalcModifiedTxPacketIPChecksumByNIC
					//  FALSE	0
					//  TRUE	1
					TryCalcModifiedTxPacketIPChecksumByNIC = (BYTE)*Value;
				}
				*Value = TryCalcModifiedTxPacketIPChecksumByNIC;
				break;
			case WINPFILTER_CTL_CODE_TX_NIC_TCP_CSUM:
				if (*ModeOrFunction == 1 && *Value <= 1) {
					// Set TryCalcModifiedTxPacketTCPChecksumByNIC
					//  FALSE	0
					//  TRUE	1
					TryCalcModifiedTxPacketTCPChecksumByNIC = (BYTE)*Value;
				}
				*Value = TryCalcModifiedTxPacketTCPChecksumByNIC;
				break;
			case WINPFILTER_CTL_CODE_TX_NIC_UDP_CSUM:
				if (*ModeOrFunction == 1 && *Value <= 1) {
					// Set TryCalcModifiedTxPacketUDPChecksumByNIC
					//  FALSE	0
					//  TRUE	1
					TryCalcModifiedTxPacketUDPChecksumByNIC = (BYTE)*Value;
				}
				*Value = TryCalcModifiedTxPacketUDPChecksumByNIC;
				break;
			default:
				Status = STATUS_UNSUCCESSFUL;
				break;
			}
			*ModeOrFunction = Function;
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
	TRACE_ENTER();
	NTSTATUS Status = STATUS_SUCCESS;

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	TRACE_EXIT();
	return Status;
}

NTSTATUS WPFilterCommDeviceClose(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	TRACE_ENTER();
	NTSTATUS Status = STATUS_SUCCESS;

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	TRACE_EXIT();
	return Status;
}

NTSTATUS WPFilterCommDeviceClean(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	TRACE_ENTER();
	NTSTATUS Status = STATUS_SUCCESS;

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	TRACE_EXIT();
	return Status;
}