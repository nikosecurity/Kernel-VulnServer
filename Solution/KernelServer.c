#pragma warning (disable : 4152) // "nonstandard extension, function/data pointer conversion in expression."
#pragma warning (disable : 4996) // "'ExAllocatePoolWithTag': ExAllocatePoolWithTag is deprecated, use ExAllocatePool2."

#include <ntddk.h>

#define DEVICE_NAME L"\\Device\\KernelServer"
#define DOS_DEVICE_NAME L"\\DosDevices\\KernelServer"

#define POOL_TAG 'liaF'

#define IOCTL_ALLOCATE_UAF_OBJECT		CTL_CODE(FILE_DEVICE_UNKNOWN, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_FREE_UAF_OBJECT			CTL_CODE(FILE_DEVICE_UNKNOWN, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_USE_UAF_OBJECT			CTL_CODE(FILE_DEVICE_UNKNOWN, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_ALLOCATE_SPRAY_OBJECT		CTL_CODE(FILE_DEVICE_UNKNOWN, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Lowkey I don't feel like implementing this function rn, but I'll keep it here for future me to deal with.
// Hopefully no one minds...
#define IOCTL_FREE_SPRAY_OBJECTS		CTL_CODE(FILE_DEVICE_UNKNOWN, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _UAF_OBJECT
{
	CHAR pRandomData[0x1000 - sizeof(PVOID)];

	PVOID pFunction;
} UAF_OBJECT, * PUAF_OBJECT;

UNICODE_STRING g_DeviceName = { 0 };
UNICODE_STRING g_DosDeviceName = { 0 };

PDEVICE_OBJECT g_pDeviceObject = 0;

PUAF_OBJECT g_pAllocation = 0;

__declspec(noinline) void DriverArbitraryFunction(void)
{
	DbgPrint("Hello World!\n");

	return;
}

NTSTATUS DriverDispatchCreateClose(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);

	NTSTATUS Status = STATUS_SUCCESS;

	pIrp->IoStatus.Status = Status;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, 0);
	return Status;
}

NTSTATUS DriverDispatchControl(PDEVICE_OBJECT pDeviceObject, PIRP pIrp)
{
	UNREFERENCED_PARAMETER(pDeviceObject);

	NTSTATUS Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION pIO = IoGetCurrentIrpStackLocation(pIrp);

	switch (pIO->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_ALLOCATE_UAF_OBJECT:
	{
		if (g_pAllocation)
		{
			ExFreePoolWithTag(g_pAllocation, POOL_TAG);
			g_pAllocation = 0;
		}

		g_pAllocation = (PUAF_OBJECT)ExAllocatePoolWithTag(NonPagedPool, sizeof(UAF_OBJECT), POOL_TAG);
		if (!g_pAllocation)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		memset(g_pAllocation, 0, sizeof(UAF_OBJECT));

		g_pAllocation->pFunction = (PVOID)DriverArbitraryFunction;

		break;
	}
	case IOCTL_FREE_UAF_OBJECT:
	{
		if (g_pAllocation)
		{
			ExFreePoolWithTag(g_pAllocation, POOL_TAG);
			// g_pAllocation = 0;
		}

		break;
	}
	case IOCTL_USE_UAF_OBJECT:
	{
		if (!g_pAllocation)
		{
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		PVOID(*pFunction)() = g_pAllocation->pFunction;
		pFunction();

		break;
	}
	case IOCTL_ALLOCATE_SPRAY_OBJECT:
	{
		PVOID pSystemBuffer = pIrp->AssociatedIrp.SystemBuffer;
		ULONG InputBufferLength = pIO->Parameters.DeviceIoControl.InputBufferLength;
		PVOID pAllocation = 0;

		if (!pSystemBuffer || InputBufferLength != sizeof(UAF_OBJECT))
		{
			Status = STATUS_INVALID_PARAMETER;
			break;
		}

		pAllocation = ExAllocatePoolWithTag(NonPagedPool, sizeof(UAF_OBJECT), POOL_TAG);
		if (!pAllocation)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		memcpy(pAllocation, pSystemBuffer, sizeof(UAF_OBJECT));

		// Yes, I'm well aware that the driver will leak this object because I am not adding it to a list (and freeing it).
		// But honestly, I don't really feel like implementing that right now.
		// If enough people use this program, then sure I'll implement it.
		// Until then... no.

		break;
	}
	case IOCTL_FREE_SPRAY_OBJECTS:
	{
		// <Insert obligatory Microsoft text like "This parameter is reserved for future use" here>
		break;
	}
	}

	pIrp->IoStatus.Status = Status;
	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, 0);
	return Status;
}

NTSTATUS DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	UNREFERENCED_PARAMETER(pDriverObject);

	IoDeleteSymbolicLink(&g_DosDeviceName);
	IoDeleteDevice(g_pDeviceObject);

	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	UNREFERENCED_PARAMETER(pRegistryPath);

	NTSTATUS Status = STATUS_SUCCESS;

	RtlInitUnicodeString(&g_DeviceName, DEVICE_NAME);
	RtlInitUnicodeString(&g_DosDeviceName, DOS_DEVICE_NAME);

	Status = IoCreateDevice(pDriverObject, 0, &g_DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, 0, &g_pDeviceObject);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	Status = IoCreateSymbolicLink(&g_DosDeviceName, &g_DeviceName);
	if (!NT_SUCCESS(Status))
	{
		IoDeleteDevice(g_pDeviceObject);
		return Status;
	}

	ASSERT(pDriverObject);
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = DriverDispatchCreateClose;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverDispatchCreateClose;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDispatchControl;

	pDriverObject->DriverUnload = DriverUnload;

	return Status;
}