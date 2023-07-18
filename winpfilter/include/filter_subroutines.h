#pragma once
#include "winpfilter.h"

NDIS_STATUS WPFilterAttach(NDIS_HANDLE NdisFilterHandle, NDIS_HANDLE FilterDriverContext, PNDIS_FILTER_ATTACH_PARAMETERS AttachParameters);
NDIS_STATUS WPFilterRestart(NDIS_HANDLE filterModuleContext, PNDIS_FILTER_RESTART_PARAMETERS RestartParameters);
NDIS_STATUS WPFilterPause(NDIS_HANDLE filterModuleContext, NDIS_FILTER_PAUSE_PARAMETERS* pauseParameters);
VOID WPFilterDetach(NDIS_HANDLE filterModuleContext);
NDIS_STATUS WPFilterSetOptions(NDIS_HANDLE  ndisFilterDriverHandle, NDIS_HANDLE  filterDriverContext);
NDIS_STATUS WPFilterSetModuleOptions(NDIS_HANDLE filterModuleContext);
VOID WPFilterReceiveFromUpper(NDIS_HANDLE FilterModuleContext, PNET_BUFFER_LIST NetBufferLists, NDIS_PORT_NUMBER PortNumber, ULONG SendFlags);
VOID WPFilterReceiveFromNIC(NDIS_HANDLE FilterModuleContext, PNET_BUFFER_LIST NetBufferLists, NDIS_PORT_NUMBER PortNumber, ULONG NumberOfNetBufferLists, ULONG ReceiveFlags);
VOID WPFilterSendToUpperFinished(NDIS_HANDLE FilterModuleContext, PNET_BUFFER_LIST NetBufferLists, ULONG ReturnFlags);
VOID WPFilterSendToNICFinished(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, ULONG sendCompleteFlags);