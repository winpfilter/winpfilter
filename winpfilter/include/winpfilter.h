/*
 * File Name:		global.h
 * Description:		Provide global constants and definition for winpfilter project
 * Author:			HBSnail
 */
#pragma once

#include <ntifs.h>
#include <wdmsec.h>
#include <ndis.h>


#define NDIS_MAJOR_VERSION 6
#define NDIS_MINOR_VERSION 20
#define DRIVER_MAJOR_VERSION 1
#define DRIVER_MINOR_VERSION 0

#define FILTER_SERVICE_NAME  L"winpfilter"
#define FILTER_GUID  L"{60857398-d67c-4a8d-8b2e-b2bc9ad03bb8}"
#define FILTER_FRIENDLY_NAME  L"winpfilter NDIS Driver"

#define WINPFILTER_COMMUNICATION_DEVICE_NAME L"\\Device\\WinpfilterCommunicationDevice"
#define WINPFILTER_COMMUNICATION_DEVICE_LINK L"\\??\\winpfilter_com_device"

#define FILTER_ALLOC_TAG 'FTAT'
#define SEND_POOL_ALLOC_TAG 'SPAT'
#define NET_PACKET_ALLOC_TAG 'APPN'

typedef UCHAR BYTE;

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
    UINT        MaxFrameSize;
    NET_IFINDEX MiniportIfIndex;

    BYTE MacAddress[NDIS_MAX_PHYS_ADDRESS_LENGTH];
    BYTE MacLength;

    NDIS_SPIN_LOCK FilterLock;    // filterLock for protection of state and outstanding sends and recvs

    FILTER_STATE FilterState;   // Which state the filter is in

    NDIS_HANDLE NBLPool;

}FILTER_CONTEXT;



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
#define TRACE_DBG(Fmt, ...)   DbgPrintEx( DPFLTR_IHVNETWORK_ID, DPFLTR_TRACE_LEVEL,   __FUNCTION__ ": " Fmt, __VA_ARGS__ )