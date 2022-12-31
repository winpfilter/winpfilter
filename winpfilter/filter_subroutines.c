/*
 * File Name:		filter_subroutines.c
 * Description:		The sub routines of filter include packet handle and others
 * Author:			HBSnail
 */

#include "winpfilter.h"
#include "filter_subroutines.h"

extern NDIS_HANDLE FilterDriverObject;
extern NDIS_HANDLE FilterDriverHandle;

extern NDIS_SPIN_LOCK FilterListLock;
extern LIST_ENTRY FilterModuleList;


VOID WPFilterFreeNBL(FILTER_CONTEXT* filterContext, NET_BUFFER_LIST* netBufferLists) {

	NET_BUFFER_LIST* pNetBufList = netBufferLists;


	while (pNetBufList != NULL) {
		NET_BUFFER* currentBuffer = NET_BUFFER_LIST_FIRST_NB(pNetBufList);
		while (currentBuffer != NULL)
		{
			//Get the NB and free
			MDL* fMDL = NET_BUFFER_FIRST_MDL(currentBuffer);

			//Our MDL chain only contains 1 MDL
			//Just get the memory & free both of them
			VOID* buffer  = MmGetSystemAddressForMdlSafe(fMDL, HighPagePriority | MdlMappingNoExecute);
			if (buffer != NULL) {
				NdisFreeMemory(buffer, 0, 0);
			}
			NdisFreeMdl(fMDL);
			currentBuffer = NET_BUFFER_NEXT_NB(currentBuffer);
		}

		//Unlink NBL one by one
		NET_BUFFER_LIST* currNBL = pNetBufList;
		NET_BUFFER_LIST_NEXT_NBL(currNBL) = NULL;
		pNetBufList = NET_BUFFER_LIST_NEXT_NBL(pNetBufList);
		//Free the NBL in processing
		NdisFreeNetBufferList(currNBL);
	}
}


NDIS_STATUS WPFilterAttach(NDIS_HANDLE ndisfilterHandle, NDIS_HANDLE filterDriverContext, NDIS_FILTER_ATTACH_PARAMETERS* attachParameters) {

	DbgPrint("WPFilterAttach\n");

	FILTER_CONTEXT* filterContext = NULL;
	NDIS_STATUS status = NDIS_STATUS_SUCCESS;

	do
	{
		if (filterDriverContext != (NDIS_HANDLE)FilterDriverObject) {
			status = NDIS_STATUS_INVALID_PARAMETER;
			break;
		}

		// We only support IEEE802.3 here but may be add others later
		if ((attachParameters->MiniportMediaType != NdisMedium802_3)
			&& (attachParameters->MiniportMediaType != NdisMediumWan)
			&& (attachParameters->MiniportMediaType != NdisMediumWirelessWan)) {

			status = NDIS_STATUS_INVALID_PARAMETER;
			break;
		}
		
		filterContext = (FILTER_CONTEXT*)NdisAllocateMemoryWithTagPriority(ndisfilterHandle, sizeof(FILTER_CONTEXT), FILTER_ALLOC_TAG, NormalPoolPriority);
		if (filterContext == NULL) {
			status = NDIS_STATUS_RESOURCES;
			break;
		}

		NdisZeroMemory(filterContext, sizeof(FILTER_CONTEXT));


		filterContext->FilterHandle = ndisfilterHandle;

		filterContext->MaxFrameSize = 0;


		NET_BUFFER_LIST_POOL_PARAMETERS PoolParameters;
		NdisZeroMemory(&PoolParameters, sizeof(NET_BUFFER_LIST_POOL_PARAMETERS));
		PoolParameters.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
		PoolParameters.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
		PoolParameters.Header.Size = sizeof(PoolParameters);
		PoolParameters.ProtocolId = NDIS_PROTOCOL_ID_DEFAULT;
		PoolParameters.ContextSize = MEMORY_ALLOCATION_ALIGNMENT;
		PoolParameters.fAllocateNetBuffer = TRUE;
		PoolParameters.PoolTag = SEND_POOL_ALLOC_TAG;
		filterContext->NBLPool = NdisAllocateNetBufferListPool(filterContext->FilterHandle, &PoolParameters);

		if (filterContext->NBLPool == NULL) {
			status = NDIS_STATUS_BAD_CHARACTERISTICS;
			break;
		}


		NdisMoveMemory(filterContext->MacAddress, attachParameters->CurrentMacAddress, NDIS_MAX_PHYS_ADDRESS_LENGTH);
		filterContext->MacLength = (BYTE)(attachParameters->MacAddressLength & 0xFF);

		filterContext->MiniportIfIndex = attachParameters->BaseMiniportIfIndex;

		NDIS_FILTER_ATTRIBUTES filterAttributes;
		NdisZeroMemory(&filterAttributes, sizeof(NDIS_FILTER_ATTRIBUTES));
		filterAttributes.Header.Revision = NDIS_FILTER_ATTRIBUTES_REVISION_1;
		filterAttributes.Header.Size = sizeof(NDIS_FILTER_ATTRIBUTES);
		filterAttributes.Header.Type = NDIS_OBJECT_TYPE_FILTER_ATTRIBUTES;
		filterAttributes.Flags = 0;


		NDIS_DECLARE_FILTER_MODULE_CONTEXT(FILTER_CONTEXT);
		status = NdisFSetAttributes(ndisfilterHandle,
			filterContext,
			&filterAttributes);

		if (status != NDIS_STATUS_SUCCESS) {
			break;
		}

		filterContext->FilterState = FilterPaused;

		NDIS_ACQUIRE_LOCK(&FilterListLock, FALSE);
		InsertHeadList(&FilterModuleList, &filterContext->FilterModuleLink);
		NDIS_RELEASE_LOCK(&FilterListLock, FALSE);

	} while (FALSE);

	if (status != NDIS_STATUS_SUCCESS) {
		if (filterContext != NULL) {

			if (filterContext->NBLPool != NULL) {
				NdisFreeNetBufferListPool(filterContext->NBLPool);
			}
			NdisFreeMemory(filterContext, 0, 0);
			filterContext = NULL;
		}
	}

	return status;
}

NDIS_STATUS WPFilterRestart(NDIS_HANDLE filterModuleContext, NDIS_FILTER_RESTART_PARAMETERS* RestartParameters) {

	NDIS_STATUS status = NDIS_STATUS_SUCCESS;
	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)filterModuleContext;


	NDIS_RESTART_ATTRIBUTES* ndisRestartAttributes = RestartParameters->RestartAttributes;
	PNDIS_RESTART_GENERAL_ATTRIBUTES GenAttr = NULL;

	status = NDIS_STATUS_SUCCESS;

	while (ndisRestartAttributes) {
		DbgPrint("%X\n", ndisRestartAttributes->Oid);
		if (ndisRestartAttributes->Oid == OID_GEN_MINIPORT_RESTART_ATTRIBUTES) {
			GenAttr = (PNDIS_RESTART_GENERAL_ATTRIBUTES)ndisRestartAttributes->Data;
			filterContext->MaxFrameSize = GenAttr->MtuSize ;
			if (status != NDIS_STATUS_SUCCESS) {
				break;
			}
		}
		ndisRestartAttributes = ndisRestartAttributes->Next;
	}


	// If everything is OK, set the filter in running state.
	filterContext->FilterState = FilterRunning; // when successful


	// Ensure the state is Paused if restart failed.
	if (status != NDIS_STATUS_SUCCESS)
	{
		filterContext->FilterState = FilterPaused;
	}

	return status;
}

NDIS_STATUS WPFilterPause(NDIS_HANDLE filterModuleContext, NDIS_FILTER_PAUSE_PARAMETERS* pauseParameters) {

	DbgPrint("WPFilterPause\n");

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)(filterModuleContext);
	NDIS_STATUS status;

	// Set the flag that the filter is going to pause
	NDIS_ACQUIRE_LOCK(&filterContext->FilterLock, FALSE);
	filterContext->FilterState = FilterPausing;
	NDIS_RELEASE_LOCK(&filterContext->FilterLock, FALSE);

	status = NDIS_STATUS_SUCCESS;

	filterContext->FilterState = FilterPaused;

	return status;
}

VOID WPFilterDetach(NDIS_HANDLE filterModuleContext) {

	DbgPrint("WPFilterDetach\n");

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)filterModuleContext;

	// Detach must not fail, so do not put any code here that can possibly fail.
	if (filterContext->NBLPool != NULL) {
		NdisFreeNetBufferListPool(filterContext->NBLPool);
	}

	NDIS_ACQUIRE_LOCK(&FilterListLock, FALSE);
	RemoveEntryList(&filterContext->FilterModuleLink);
	NDIS_RELEASE_LOCK(&FilterListLock, FALSE);

	// Free the memory allocated
	NdisFreeMemory(filterContext, 0, 0);
	filterContext = NULL;
	return;
}

NDIS_STATUS WPFilterSetOptions(NDIS_HANDLE  ndisFilterDriverHandle, NDIS_HANDLE  filterDriverContext) {

	if ((ndisFilterDriverHandle != (NDIS_HANDLE)FilterDriverHandle) || (filterDriverContext != (NDIS_HANDLE)FilterDriverObject))
	{
		return NDIS_STATUS_INVALID_PARAMETER;
	}

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS WPFilterSetModuleOptions(NDIS_HANDLE filterModuleContext) {

	NDIS_STATUS status = NDIS_STATUS_SUCCESS;

	return status;
}

VOID WPFilterReceiveFromUpper(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, NDIS_PORT_NUMBER portNumber, ULONG sendFlags) {

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)filterModuleContext;
	BOOLEAN dispatchLevel;

	for (NET_BUFFER_LIST* currNBL = netBufferLists; currNBL != NULL; currNBL = NET_BUFFER_LIST_NEXT_NBL(currNBL)) {
		for (NET_BUFFER* currNB = NET_BUFFER_LIST_FIRST_NB(currNBL); currNB != NULL; currNB = NET_BUFFER_NEXT_NB(currNB)) {
			if (currNB->DataLength <= 0) {
				//It must not occurr
				continue;
			}
			
			BYTE* pPacket = ExAllocatePoolWithTag(NonPagedPoolNx, currNB->DataLength, NET_PACKET_ALLOC_TAG);
			if (pPacket == NULL) {
				//Error occurr
				continue;
			}	
			BYTE* ethDataPtr = NdisGetDataBuffer(currNB, currNB->DataLength, pPacket, 1, 0);
			//If data in NB is contiguous, system will not auto copy the data, which means we should copy them manually
			if (ethDataPtr != (pPacket)) {
				NdisMoveMemory((BYTE*)pPacket, ethDataPtr, currNB->DataLength);
			}


			ExFreePoolWithTag(pPacket, NET_PACKET_ALLOC_TAG);
		}
	}


	//NdisFSendNetBufferListsComplete(filterContext->FilterHandle, netBufferLists, sendFlags);
	NdisFSendNetBufferLists(filterContext->FilterHandle, netBufferLists, portNumber, sendFlags);

}

VOID WPFilterReceiveFromNIC(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, NDIS_PORT_NUMBER portNumber, ULONG numberOfNetBufferLists, ULONG receiveFlags) {

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)filterModuleContext;
	BOOLEAN dispatchLevel;
	ULONG returnFlags = 0;


	dispatchLevel = NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(receiveFlags);

	if (dispatchLevel)
	{
		NDIS_SET_RETURN_FLAG(returnFlags, NDIS_RETURN_FLAGS_DISPATCH_LEVEL);
	}

	NET_BUFFER_LIST* acceptList = NULL;
	NET_BUFFER_LIST* dropList = NULL;

	for (NET_BUFFER_LIST* currNBL = netBufferLists; currNBL != NULL; currNBL = NET_BUFFER_LIST_NEXT_NBL(currNBL)) {
		for (NET_BUFFER* currNB = NET_BUFFER_LIST_FIRST_NB(currNBL); currNB != NULL; currNB = NET_BUFFER_NEXT_NB(currNB)) {
			if (currNB->DataLength <= 0) {
				//It must not occurr
				continue;
			}

			BYTE* pPacket = ExAllocatePoolWithTag(NonPagedPoolNx, currNB->DataLength , NET_PACKET_ALLOC_TAG);
			if (pPacket == NULL) {
				//Error occurr
				continue;
			}
			BYTE* ethDataPtr = NdisGetDataBuffer(currNB, currNB->DataLength, pPacket, 1, 0);
			//If data in NB is contiguous, system will not auto copy the data, which means we should copy them manually
			if (ethDataPtr != (pPacket )) {
				NdisMoveMemory((BYTE*)pPacket, ethDataPtr, currNB->DataLength);
			}

			ExFreePoolWithTag(pPacket, NET_PACKET_ALLOC_TAG);
		}
	}

	//NdisFReturnNetBufferLists(filterContext->FilterHandle, netBufferLists, receiveFlags);

	NdisFIndicateReceiveNetBufferLists(filterContext->FilterHandle,netBufferLists,portNumber,numberOfNetBufferLists,receiveFlags);

}


VOID WPFilterSendToUpperFinished(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, ULONG returnFlags) {

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)filterModuleContext;

	NdisFReturnNetBufferLists(filterContext->FilterHandle, netBufferLists, returnFlags);
}


VOID WPFilterSendToNICFinished(NDIS_HANDLE filterModuleContext, NET_BUFFER_LIST* netBufferLists, ULONG sendCompleteFlags) {

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)filterModuleContext;


	NdisFSendNetBufferListsComplete(filterContext->FilterHandle, netBufferLists, sendCompleteFlags);
}
