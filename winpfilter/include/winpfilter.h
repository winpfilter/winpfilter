/*
 * File Name:		winpfilter.h
 * Description:		Provide global constants and definition for winpfilter project
 * Author:			HBSnail
 */
#pragma once

#include <ntifs.h>
#include <wdmsec.h>
#include <netioapi.h>
#include <ndis.h>
#include <limits.h>

#define WINPFILTER_VERSION 20230814

#define NDIS_MAJOR_VERSION 6
#define NDIS_MINOR_VERSION 20
#define DRIVER_MAJOR_VERSION 1
#define DRIVER_MINOR_VERSION 0

#define FILTER_SERVICE_NAME  L"winpfilter"
#define FILTER_GUID  L"{60857398-d67c-4a8d-8b2e-b2bc9ad03bb8}"
#define FILTER_FRIENDLY_NAME  L"winpfilter NDIS Driver"

#define WINPFILTER_R0HOOK_COMMUNICATION_DEVICE_NAME L"\\Device\\WinpfilterR0HookCommunicationDevice"
#define WINPFILTER_R0HOOK_COMMUNICATION_DEVICE_LINK L"\\??\\winpfilter_r0hook_com_device"
#define WINPFILTER_R3COMMAND_COMMUNICATION_DEVICE_NAME L"\\Device\\WinpfilterR3CommandCommunicationDevice"
#define WINPFILTER_R3COMMAND_COMMUNICATION_DEVICE_LINK L"\\??\\winpfilter_r3cmd_com_device"

#define FILTER_CONTEXT_ALLOC_TAG 'TACF'
#define NET_BUFFER_LIST_POOL_ALLOC_TAG 'PLBN'
#define NET_PACKET_ALLOC_TAG 'APPN'
#define ROUTING_TASK_ALLOC_TAG 'ATTR'
#define IP_LINK_ALLOC_TAG 'TALI'
#define ROUTE_TABLE_ALLOC_TAG 'TATR'
#define HOOK_ENTRY_ALLOC_TAG 'TAEH'

typedef UCHAR BYTE;
#define BYTE_MIN_VALUE 0
#define BYTE_BIT_LENGTH 8
#define BYTE_MAX_VALUE 255

typedef enum _FILTER_STATE {
    FilterStateUnspecified,
    FilterInitialized,
    FilterPausing,
    FilterPaused,
    FilterRunning,
    FilterRestarting,
    FilterDetaching
} FILTER_STATE;




typedef struct _FILTER_CONTEXT {

    LIST_ENTRY FilterModuleLink;

    //Reference to this filter
    NDIS_HANDLE FilterHandle;
    ULONG       MaxFrameSize;
    NET_LUID    BasePortLuid;
    IF_INDEX    BasePortIndex;
    PNDIS_OFFLOAD OffloadConfig;

    BYTE MacAddress[NDIS_MAX_PHYS_ADDRESS_LENGTH];
    BYTE MacLength;

    NDIS_HANDLE NBLPool;

    NDIS_SPIN_LOCK FilterLock;    // filterLock for protection of state and outstanding sends and recvs
    FILTER_STATE FilterState;   // Which state the filter is in

}FILTER_CONTEXT,*PFILTER_CONTEXT;



#define NDIS_INIT_LOCK(_pLock)      NdisAllocateSpinLock(_pLock)

#define NDIS_FREE_LOCK(_pLock)      NdisFreeSpinLock(_pLock)

#define NDIS_ACQUIRE_LOCK(_pLock, DispatchLevel)              \
        {                                                           \
            if (DispatchLevel)                                      \
            {                                                       \
                NdisDprAcquireSpinLock(_pLock);                     \
            }                                                       \
            else                                                    \
            {                                                       \
                NdisAcquireSpinLock(_pLock);                        \
            }                                                       \
        }


#define NDIS_RELEASE_LOCK(_pLock, DispatchLevel)              \
        {                                                           \
            if (DispatchLevel)                                      \
            {                                                       \
                NdisDprReleaseSpinLock(_pLock);                     \
            }                                                       \
            else                                                    \
            {                                                       \
                NdisReleaseSpinLock(_pLock);                        \
            }                                                       \
        }

#define TRACE_ENTER() TRACE_DBG("ENTER\n")
#define TRACE_EXIT()  TRACE_DBG("EXIT\n")
#ifdef DBG
#define TRACE_DBG(Fmt, ...)   DbgPrintEx( DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL,   __FUNCTION__ ": " Fmt, __VA_ARGS__ )
#else
#define TRACE_DBG(Fmt, ...) 
#endif
