/*
 * File Name:		filter_subroutines.c
 * Description:		The sub routines of filter include packet handle and others
 * Author:			HBSnail
 */

#include "filter_subroutines.h"
#include "hook_manager.h"
#include "route.h"
#include "global_variables.h"
#include "routing_engine.h"
#include "ether.h"
#include "ip.h"

VOID FreeSingleNBL(PNET_BUFFER_LIST NetBufferList);

// Remove and free all NBL allocated with FilterContext->FilterHandle in an NBL chain.
// This function will return the head of the NBL chain 
PNET_BUFFER_LIST RemoveNBLsAllocatedByFilterFromNBLChain(PFILTER_CONTEXT FilterContext, PNET_BUFFER_LIST NetBufferListChain) {
	PNET_BUFFER_LIST NBLChainHeader = NetBufferListChain;
	PNET_BUFFER_LIST CurrentNBL = NetBufferListChain;
	PNET_BUFFER_LIST PreviousNBL = NULL;
	PNET_BUFFER_LIST NextNBL = NULL;

	while (CurrentNBL != NULL) {
		NextNBL = NET_BUFFER_LIST_NEXT_NBL(CurrentNBL);

		if (CurrentNBL->SourceHandle == FilterContext->FilterHandle) {
			// This NBL allocated by us.
			// Free it!
			if (CurrentNBL == NBLChainHeader) {
				NBLChainHeader = NextNBL;
			}
			else {
				PreviousNBL->Next = NextNBL;
			}
			CurrentNBL->Next = NULL;
			FreeSingleNBL(CurrentNBL);
			CurrentNBL = NextNBL;

			continue;
		}

		PreviousNBL = CurrentNBL;
		CurrentNBL = NextNBL;
	}

	return NBLChainHeader;
}

VOID FreeSingleNBL(PNET_BUFFER_LIST NetBufferList) {
	PNET_BUFFER_LIST CurrentNBL = NetBufferList;
	PNET_BUFFER CurrentNetBuffer = NET_BUFFER_LIST_FIRST_NB(NetBufferList);
	PMDL CurrentMDL;
	PVOID DataBuffer;

	while (CurrentNetBuffer != NULL)
	{
		// Get the NB and free
		CurrentMDL = NET_BUFFER_FIRST_MDL(CurrentNetBuffer);
		if (CurrentMDL != NULL) {
			// Our MDL chain only contains 1 MDL
			// Just get the memory & free both of them 
			DataBuffer = MmGetSystemAddressForMdlSafe(CurrentMDL, HighPagePriority | MdlMappingNoExecute);
			if (DataBuffer != NULL) {
				ExFreePool(DataBuffer);
			}
			NdisFreeMdl(CurrentMDL);
		}
		CurrentNetBuffer = NET_BUFFER_NEXT_NB(CurrentNetBuffer);
	}

	// Free the NBL in
	NdisFreeNetBufferList(CurrentNBL);
}


NDIS_STATUS WPFilterAttach(NDIS_HANDLE NdisFilterHandle, NDIS_HANDLE FilterDriverContext, PNDIS_FILTER_ATTACH_PARAMETERS AttachParameters) {

	DbgPrint("WPFilterAttach\n");

	PFILTER_CONTEXT FilterContext = NULL;
	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

	do
	{
		if (FilterDriverContext != (NDIS_HANDLE)FilterDriverObject) {
			Status = NDIS_STATUS_INVALID_PARAMETER;
			break;
		}

		// We only support IEEE802.3 here but may be add others later
		if ((AttachParameters->MiniportMediaType != NdisMedium802_3)
			&& (AttachParameters->MiniportMediaType != NdisMediumWan)
			&& (AttachParameters->MiniportMediaType != NdisMediumWirelessWan)) {
			Status = NDIS_STATUS_INVALID_PARAMETER;
			break;
		}

		FilterContext = (PFILTER_CONTEXT)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(FILTER_CONTEXT), FILTER_CONTEXT_ALLOC_TAG);
		if (FilterContext == NULL) {
			Status = NDIS_STATUS_RESOURCES;
			break;
		}

		NdisZeroMemory(FilterContext, sizeof(FILTER_CONTEXT));


		FilterContext->FilterHandle = NdisFilterHandle;
		FilterContext->MaxFrameSize = 0;



		NET_BUFFER_LIST_POOL_PARAMETERS PoolParameters;
		NdisZeroMemory(&PoolParameters, sizeof(NET_BUFFER_LIST_POOL_PARAMETERS));
		PoolParameters.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
		PoolParameters.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
		PoolParameters.Header.Size = sizeof(PoolParameters);
		PoolParameters.ProtocolId = NDIS_PROTOCOL_ID_DEFAULT;
		PoolParameters.ContextSize = MEMORY_ALLOCATION_ALIGNMENT;
		PoolParameters.fAllocateNetBuffer = TRUE;
		PoolParameters.PoolTag = NET_BUFFER_LIST_POOL_ALLOC_TAG;
		FilterContext->NBLPool = NdisAllocateNetBufferListPool(FilterContext->FilterHandle, &PoolParameters);

		if (FilterContext->NBLPool == NULL) {
			Status = NDIS_STATUS_BAD_CHARACTERISTICS;
			break;
		}


		NdisMoveMemory(FilterContext->MacAddress, AttachParameters->CurrentMacAddress, NDIS_MAX_PHYS_ADDRESS_LENGTH);
		FilterContext->MacLength = (BYTE)(AttachParameters->MacAddressLength & 0xFF);
		FilterContext->BasePortLuid = AttachParameters->BaseMiniportNetLuid;
		FilterContext->BasePortIndex = AttachParameters->BaseMiniportIfIndex;
		FilterContext->OffloadConfig = AttachParameters->DefaultOffloadConfiguration;

		NDIS_FILTER_ATTRIBUTES FilterAttributes;
		NdisZeroMemory(&FilterAttributes, sizeof(NDIS_FILTER_ATTRIBUTES));
		FilterAttributes.Header.Revision = NDIS_FILTER_ATTRIBUTES_REVISION_1;
		FilterAttributes.Header.Size = sizeof(NDIS_FILTER_ATTRIBUTES);
		FilterAttributes.Header.Type = NDIS_OBJECT_TYPE_FILTER_ATTRIBUTES;
		FilterAttributes.Flags = 0;


		NDIS_DECLARE_FILTER_MODULE_CONTEXT(FILTER_CONTEXT);
		Status = NdisFSetAttributes(NdisFilterHandle,
			FilterContext,
			&FilterAttributes);

		if (!NT_SUCCESS(Status)) {
			break;
		}

		FilterContext->FilterState = FilterPaused;

		NDIS_ACQUIRE_LOCK(&FilterListLock, FALSE);
		InsertHeadList(&FilterModuleList, &FilterContext->FilterModuleLink);
		NDIS_RELEASE_LOCK(&FilterListLock, FALSE);

	} while (FALSE);

	if (!NT_SUCCESS(Status)) {
		if (FilterContext != NULL) {
			if (FilterContext->NBLPool != NULL) {
				NdisFreeNetBufferListPool(FilterContext->NBLPool);
			}
			ExFreePoolWithTag(FilterContext, FILTER_CONTEXT_ALLOC_TAG);
			FilterContext = NULL;
		}
	}
	return Status;
}

NDIS_STATUS WPFilterRestart(NDIS_HANDLE FilterModuleContext, PNDIS_FILTER_RESTART_PARAMETERS RestartParameters) {

	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
	PFILTER_CONTEXT FilterContext = (PFILTER_CONTEXT)FilterModuleContext;

	PNDIS_RESTART_ATTRIBUTES NdisRestartAttributes = RestartParameters->RestartAttributes;
	PNDIS_RESTART_GENERAL_ATTRIBUTES GenAttr = NULL;

	Status = NDIS_STATUS_SUCCESS;

	while (NdisRestartAttributes) {
		DbgPrint("%X\n", NdisRestartAttributes->Oid);
		if (NdisRestartAttributes->Oid == OID_GEN_MINIPORT_RESTART_ATTRIBUTES) {
			GenAttr = (PNDIS_RESTART_GENERAL_ATTRIBUTES)NdisRestartAttributes->Data;
			FilterContext->MaxFrameSize = GenAttr->MtuSize + sizeof(ETH_HEADER);
			if (Status != NDIS_STATUS_SUCCESS) {
				break;
			}
		}
		NdisRestartAttributes = NdisRestartAttributes->Next;
	}


	// If everything is OK, set the filter in running state.
	FilterContext->FilterState = FilterRunning; // when successful


	// Ensure the state is Paused if restart failed.
	if (Status != NDIS_STATUS_SUCCESS)
	{
		FilterContext->FilterState = FilterPaused;
	}

	return Status;
}

NDIS_STATUS WPFilterPause(NDIS_HANDLE FilterModuleContext, NDIS_FILTER_PAUSE_PARAMETERS* PauseParameters) {

	TRACE_ENTER();

	PFILTER_CONTEXT FilterContext = (PFILTER_CONTEXT)(FilterModuleContext);
	NDIS_STATUS Status;

	// Set the flag that the filter is going to pause
	NDIS_ACQUIRE_LOCK(&FilterContext->FilterLock, FALSE);
	FilterContext->FilterState = FilterPausing;
	Status = NDIS_STATUS_SUCCESS;
	FilterContext->FilterState = FilterPaused;
	NDIS_RELEASE_LOCK(&FilterContext->FilterLock, FALSE);

	TRACE_EXIT();
	return Status;
}

VOID WPFilterDetach(NDIS_HANDLE FilterModuleContext) {

	TRACE_ENTER();

	PFILTER_CONTEXT FilterContext = (PFILTER_CONTEXT)FilterModuleContext;

	// Detach must not fail, so do not put any code here that can possibly fail.
	if (FilterContext->NBLPool != NULL) {
		NdisFreeNetBufferListPool(FilterContext->NBLPool);
	}

	NDIS_ACQUIRE_LOCK(&FilterListLock, FALSE);
	RemoveEntryList(&FilterContext->FilterModuleLink);
	NDIS_RELEASE_LOCK(&FilterListLock, FALSE);

	// Free the memory allocated
	ExFreePoolWithTag(FilterContext, FILTER_CONTEXT_ALLOC_TAG);
	FilterContext = NULL;

	TRACE_EXIT();
	return;
}

NDIS_STATUS WPFilterSetOptions(NDIS_HANDLE  ndisFilterDriverHandle, NDIS_HANDLE  filterDriverContext) {

	if ((ndisFilterDriverHandle != (NDIS_HANDLE)FilterDriverHandle) || (filterDriverContext != (NDIS_HANDLE)FilterDriverObject))
	{
		return NDIS_STATUS_INVALID_PARAMETER;
	}

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS WPFilterSetModuleOptions(NDIS_HANDLE FilterModuleContext) {

	NDIS_STATUS status = NDIS_STATUS_SUCCESS;

	return status;
}

inline VOID LinkSingleNBLIntoNBLChainTail(PNET_BUFFER_LIST* pChainHead, PNET_BUFFER_LIST* pChainTail, PNET_BUFFER_LIST SingleNBLNode)
{
	NET_BUFFER_LIST_NEXT_NBL(SingleNBLNode) = NULL;
	if (*pChainHead == NULL || *pChainTail == NULL) {
		*pChainHead = SingleNBLNode;
		*pChainTail = *pChainHead;
	}
	else {
		NET_BUFFER_LIST_NEXT_NBL(*pChainTail) = SingleNBLNode;
		*pChainTail = NET_BUFFER_LIST_NEXT_NBL(*pChainTail);
	}
}


inline VOID LinkSingleNBLIntoNBLChainHead(PNET_BUFFER_LIST* pChainHead, PNET_BUFFER_LIST SingleNBLNode) {
	NET_BUFFER_LIST_NEXT_NBL(SingleNBLNode) = *pChainHead;
	*pChainHead = SingleNBLNode;
}

inline VOID RemoveSingleNBLFromNBLChainHead(PNET_BUFFER_LIST* pChainHead) {
	if (*pChainHead != NULL) {
		*pChainHead = NET_BUFFER_LIST_NEXT_NBL(*pChainHead);
	}
}

inline VOID DropPacket(BOOLEAN CanNotPend, PNET_BUFFER_LIST* NBLDropListHeader, PNET_BUFFER_LIST SingleNBLPendingDrop, ULONG* DropCounter, BOOLEAN* DroppedFlag) {
	if (!(*DroppedFlag)) {
		if (!CanNotPend) {
			LinkSingleNBLIntoNBLChainHead(NBLDropListHeader, SingleNBLPendingDrop);
		}
		(*DropCounter)++;
		*DroppedFlag = TRUE;
	}
}

VOID WPFilterReceiveFromUpper(NDIS_HANDLE FilterModuleContext, PNET_BUFFER_LIST NetBufferLists, NDIS_PORT_NUMBER PortNumber, ULONG SendFlags) {

	PFILTER_CONTEXT FilterContext = (PFILTER_CONTEXT)FilterModuleContext;
	BOOLEAN DispatchLevel = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags);

	PNET_BUFFER_LIST CurrentNBL = NetBufferLists;
	PNET_BUFFER_LIST NextNBL = NULL;

	while (CurrentNBL != NULL) {

		NextNBL = NET_BUFFER_LIST_NEXT_NBL(CurrentNBL);

		for (NET_BUFFER* CurrentNB = NET_BUFFER_LIST_FIRST_NB(CurrentNBL); CurrentNB != NULL; CurrentNB = NET_BUFFER_NEXT_NB(CurrentNB)) {

			BYTE* EthDataPtr;
			ULONG DataLength = CurrentNB->DataLength;
			ULONG PacketBufferLength = DataLength;
			BYTE* PacketBuffer = NULL;

			if (DataLength == 0) {
				continue;
			}
			PacketBuffer = ExAllocatePoolWithTag(NonPagedPoolNx, PacketBufferLength, NET_PACKET_ALLOC_TAG);
			if (PacketBuffer == NULL) {
				continue;
			}
			RtlZeroMemory(PacketBuffer, PacketBufferLength);
			EthDataPtr = NdisGetDataBuffer(CurrentNB, DataLength, PacketBuffer, 1, 0);
			//If data in NB is contiguous, system will not auto copy the data, which means we should copy them manually
			if (EthDataPtr != (PacketBuffer)) {
				RtlCopyMemory(PacketBuffer, EthDataPtr, DataLength);
			}
			NTSTATUS Status =
				CreateAndProcessRoutingTaskWithoutDataBufferCopy(PacketBuffer, CurrentNB->DataLength, CurrentNB->DataLength, FILTER_POINT_OUTPUT, FilterContext, CurrentNBL);
			if (!NT_SUCCESS(Status)) {
				ExFreePoolWithTag(PacketBuffer, NET_PACKET_ALLOC_TAG);
			}

		}

		CurrentNBL = NextNBL;
	}

	NdisFSendNetBufferListsComplete(FilterContext->FilterHandle, NetBufferLists, SendFlags);
}

VOID WPFilterReceiveFromNIC(NDIS_HANDLE FilterModuleContext, PNET_BUFFER_LIST NetBufferLists, NDIS_PORT_NUMBER PortNumber, ULONG NumberOfNetBufferLists, ULONG ReceiveFlags) {

	PFILTER_CONTEXT FilterContext = (PFILTER_CONTEXT)FilterModuleContext;
	BOOLEAN CanNotPend = NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags);
	BOOLEAN DispatchLevel = NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags);

	PNET_BUFFER_LIST CurrentNBL = NetBufferLists;

	while (CurrentNBL != NULL) {
		// Note: on the receive path for Ethernet packets, one NBL will have exactly one NB. 
		// So you do not need to worry about dropping one NB, 
		// but trying to indicate up the remaining NBs on the same NBL.
		// In other words, if the first NB should be dropped, drop the whole NBL.
		PNET_BUFFER CurrentNB = NET_BUFFER_LIST_FIRST_NB(CurrentNBL);

		PNET_BUFFER_LIST NextNBL = NET_BUFFER_LIST_NEXT_NBL(CurrentNBL);


		do
		{
			if (CurrentNB->DataLength == 0) {
				// It must not occurr. Return the NBL.
				break;
			}

			BYTE* PacketBuffer = ExAllocatePoolWithTag(NonPagedPoolNx, CurrentNB->DataLength, NET_PACKET_ALLOC_TAG);
			if (PacketBuffer == NULL) {
				// Allocate failed. DROP!
				break;
			}

			RtlZeroMemory(PacketBuffer, CurrentNB->DataLength);
			BYTE* EthDataPtr = NdisGetDataBuffer(CurrentNB, CurrentNB->DataLength, PacketBuffer, 1, 0);
			// If data in NB is contiguous, 
			// system will not auto copy the data, which means we should copy them manually
			if (EthDataPtr != (PacketBuffer)) {
				RtlCopyMemory(PacketBuffer, EthDataPtr, CurrentNB->DataLength);
			}

			NTSTATUS Status =
				CreateAndProcessRoutingTaskWithoutDataBufferCopy(PacketBuffer, CurrentNB->DataLength, CurrentNB->DataLength, FILTER_POINT_PREROUTING, FilterContext, CurrentNBL);
			if (!NT_SUCCESS(Status)) {
				ExFreePoolWithTag(PacketBuffer, NET_PACKET_ALLOC_TAG);
			}

		} while (FALSE);

		CurrentNBL = NextNBL;
	}

	TRACE_DBG("CanNotPend:%d\n", CanNotPend);
	TRACE_DBG("TotalCnt:%d\n", NumberOfNetBufferLists);

	// if (CanNotPend) :
	// Return from this function.
	if (CanNotPend) {
		return;
	}
	// Drop all the packtes.
	NdisFReturnNetBufferLists(FilterContext->FilterHandle, NetBufferLists, ReceiveFlags);

}


VOID WPFilterSendToUpperFinished(NDIS_HANDLE FilterModuleContext, PNET_BUFFER_LIST NetBufferLists, ULONG ReturnFlags) {

	PFILTER_CONTEXT FilterContext = (PFILTER_CONTEXT)FilterModuleContext;
	PNET_BUFFER_LIST NBLHeader = NetBufferLists;

	// If your filter injected any send packets,
	// you MUST identify their NBLs here and remove them from the chain.
	NBLHeader = RemoveNBLsAllocatedByFilterFromNBLChain(FilterContext, NBLHeader);

	// Return the received NBLs.  
	// If you removed any NBLs from the chain, make sure the chain isn't empty.
	if (NBLHeader == NULL) {
		return;
	}
	NdisFReturnNetBufferLists(FilterContext->FilterHandle, NBLHeader, ReturnFlags);
}


VOID WPFilterSendToNICFinished(NDIS_HANDLE FilterModuleContext, NET_BUFFER_LIST* NetBufferLists, ULONG SendCompleteFlags) {

	PFILTER_CONTEXT FilterContext = (PFILTER_CONTEXT)FilterModuleContext;
	PNET_BUFFER_LIST NBLHeader = NetBufferLists;

	// If your filter injected any send packets,
	// you MUST identify their NBLs here and remove them from the chain.
	NBLHeader = RemoveNBLsAllocatedByFilterFromNBLChain(FilterContext, NBLHeader);

	// Send complete the NBLs.  
	// If you removed any NBLs from the chain, make sure the chain isn't empty.
	if (NBLHeader == NULL) {
		return;
	}
	NdisFSendNetBufferListsComplete(FilterContext->FilterHandle, NetBufferLists, SendCompleteFlags);
}
