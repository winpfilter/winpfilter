#include "communication.h"
#include "global_variables.h"

NTSTATUS WPFilterCommDeviceIOCtl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS Status = STATUS_SUCCESS;

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	if (DeviceObject == WPFilterR0HookCommunicationDevice) {
		// Used for hook management
		  
	}
	else if (DeviceObject == WPFilterR0HookCommunicationDevice) {
		// Used for command

	}
	else {
		// Error
		Status = STATUS_UNSUCCESSFUL;
	}

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS WPFilterCommDeviceRead(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS Status = STATUS_SUCCESS;

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	if (DeviceObject == WPFilterR0HookCommunicationDevice) {
		// Used for hook management
		Status = STATUS_UNSUCCESSFUL;
	}
	else if (DeviceObject == WPFilterR0HookCommunicationDevice) {
		// Used for command
		Status = STATUS_UNSUCCESSFUL;
	}
	else {
		// Error
		Status = STATUS_UNSUCCESSFUL;
	}

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS WPFilterCommDeviceWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS Status = STATUS_SUCCESS;

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	if (DeviceObject == WPFilterR0HookCommunicationDevice) {
		// Used for hook management
		Status = STATUS_UNSUCCESSFUL;
	}
	else if (DeviceObject == WPFilterR0HookCommunicationDevice) {
		// Used for command
		Status = STATUS_UNSUCCESSFUL;
	}
	else {
		// Error
		Status = STATUS_UNSUCCESSFUL;
	}

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
