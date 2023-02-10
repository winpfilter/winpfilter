#pragma once
#include "winpfilter.h"

NTSTATUS WPFilterCommDeviceIOCtl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS WPFilterCommDeviceRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS WPFilterCommDeviceWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);