#pragma once
#include "winpfilter.h"
NDIS_STATUS WPFilterAttach(NDIS_HANDLE ndisfilterHandle, NDIS_HANDLE filterDriverContext, NDIS_FILTER_ATTACH_PARAMETERS* attachParameters);
NDIS_STATUS WPFilterRestart(NDIS_HANDLE filterModuleContext, NDIS_FILTER_RESTART_PARAMETERS* RestartParameters);
NDIS_STATUS WPFilterPause(NDIS_HANDLE filterModuleContext, NDIS_FILTER_PAUSE_PARAMETERS* pauseParameters);
VOID WPFilterDetach(NDIS_HANDLE filterModuleContext);
NDIS_STATUS WPFilterSetOptions(NDIS_HANDLE  ndisFilterDriverHandle, NDIS_HANDLE  filterDriverContext);
NDIS_STATUS WPFilterSetModuleOptions(NDIS_HANDLE filterModuleContext);
VOID WPFilterReceiveFromUpper(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, NDIS_PORT_NUMBER portNumber, ULONG sendFlags);
VOID WPFilterReceiveFromNIC(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, NDIS_PORT_NUMBER portNumber, ULONG numberOfNetBufferLists, ULONG receiveFlags);
VOID WPFilterSendToUpperFinished(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, ULONG returnFlags);
VOID WPFilterSendToNICFinished(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, ULONG sendCompleteFlags);