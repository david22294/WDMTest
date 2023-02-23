#include<wdm.h>

extern "C" DRIVER_INITIALIZE DriverEntry;
extern "C" DRIVER_UNLOAD DriverUnload;
extern "C" DRIVER_STARTIO DriverStartIo;
IO_WORKITEM_ROUTINE IoWorkitemRoutine;

PDEVICE_OBJECT g_pDeviceObject = NULL;
PVOID pThreadHandle1;
PVOID pThreadHandle2;
KEVENT EventCloseThread;

#define waitTime -10 * 30000000 // 30 seconds

void ThreadFunc1(PVOID StartContext) 
{
    UNREFERENCED_PARAMETER(StartContext);
    DbgPrint("Hello Kernel Thread 1!\n");
    DbgPrint("Kernel Thread 1 starts working!\n");
    LARGE_INTEGER wait;
    wait.QuadPart = waitTime;
    while (true) {
        if (KeWaitForSingleObject(&EventCloseThread, Executive, KernelMode, FALSE, &wait) == STATUS_SUCCESS)
            break;
        DbgPrint("Kernel Thread 1 is working!\n");
    }
    // DbgBreakPoint();
    DbgPrint("Bye Kerenl Thread 1!\n");
    PsTerminateSystemThread(STATUS_SUCCESS);
}

void ThreadFunc2(PVOID StartContext)
{
    UNREFERENCED_PARAMETER(StartContext);
    DbgPrint("Hello Kernel Thread 2!\n");
    DbgPrint("Kernel Thread 2 starts working!\n");
    LARGE_INTEGER wait;
    wait.QuadPart = waitTime;
    while (true) {
        if (KeWaitForSingleObject(&EventCloseThread, Executive, KernelMode, FALSE, &wait) == STATUS_SUCCESS)
            break;
        DbgPrint("Kernel Thread 2 is working!\n");
    }
    // DbgBreakPoint();
    DbgPrint("Bye Kerenl Thread 2!\n");
    PsTerminateSystemThread(STATUS_SUCCESS);
}

extern "C" NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status = STATUS_SUCCESS;

    DriverObject->DriverUnload = DriverUnload;

    DbgPrint("Hello, Driver!\n");

    status = IoCreateDevice(
        DriverObject,
        0,
        NULL,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_pDeviceObject);
    if(!NT_SUCCESS(status))
        return status;

    DbgPrint("Hi, Device!\n");
   

    //Practice of creating WorkItem
    PIO_WORKITEM workItem = NULL;
    if (NT_SUCCESS(status)) {
        workItem = IoAllocateWorkItem(g_pDeviceObject);
        if (workItem != NULL) {
            IoInitializeWorkItem(g_pDeviceObject, workItem);
            IoQueueWorkItem(workItem, IoWorkitemRoutine, DelayedWorkQueue, workItem);
        }
        else {
            DbgPrint("Create WorkItem Fail!\n");
        }
    }
    else {
        DbgPrint("Create DeviceObject Fail!\n");
    }

    // Practice of creating System Thread.
    KeInitializeEvent(&EventCloseThread, NotificationEvent, FALSE);
    HANDLE hThreadHandle = NULL;
    status = PsCreateSystemThread(&hThreadHandle, GENERIC_ALL, NULL, NULL, NULL, ThreadFunc1, NULL);
    if (status == STATUS_SUCCESS) {
        ObReferenceObjectByHandle(hThreadHandle, GENERIC_ALL, NULL, KernelMode, &pThreadHandle1, NULL);
        ZwClose(hThreadHandle);
    }

    return status;
}

extern "C" void
DriverUnload(
    PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    
    KeSetEvent(&EventCloseThread, IO_NO_INCREMENT, FALSE);
    if (pThreadHandle1 != NULL) {
        KeWaitForSingleObject(pThreadHandle1, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(pThreadHandle1);
        DbgPrint("Free Thread 1 Handle!\n");
    }

    if (pThreadHandle2 != NULL) {
        KeWaitForSingleObject(pThreadHandle2, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(pThreadHandle2);
        DbgPrint("Free Thread 2 Handle!\n");
    }

    if (g_pDeviceObject) {
        IoDeleteDevice(g_pDeviceObject);
        DbgPrint("Bye, Device!\n");
    }

    DbgPrint("Bye, Driver!\n");
    return;
}

extern "C" void 
DriverStartIo(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    
    return;
}

void IoWorkitemRoutine(
    PDEVICE_OBJECT DeviceObject,
    PVOID Context
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);

    DbgPrint("Hello WorkItem\n");

    HANDLE hThreadHandle = NULL;
    NTSTATUS status = PsCreateSystemThread(&hThreadHandle, GENERIC_ALL, NULL, NULL, NULL, ThreadFunc2, NULL);
    if (status == STATUS_SUCCESS) {
        ObReferenceObjectByHandle(hThreadHandle, GENERIC_ALL, NULL, KernelMode, &pThreadHandle2, NULL);
        ZwClose(hThreadHandle);
    }

    if (Context != NULL) {
        IoUninitializeWorkItem((PIO_WORKITEM)Context);
        IoFreeWorkItem((PIO_WORKITEM)Context);
        DbgPrint("Free WorkItem!\n");
    }
    DbgPrint("WorkItem Routine Done\n");
    return;
}