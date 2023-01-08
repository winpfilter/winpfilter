/*
 * File Name:		filter_subroutines.c
 * Description:		The sub routines of filter include packet handle and others
 * Author:			HBSnail
 */

#include "winpfilter.h"
#include "filter_subroutines.h"
#include "hook_manager.h"
#include "net/ether.h"

extern NDIS_HANDLE FilterDriverObject;
extern NDIS_HANDLE FilterDriverHandle;

extern NDIS_SPIN_LOCK FilterListLock;
extern LIST_ENTRY FilterModuleList;

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
				ExFreePoolWithTag(DataBuffer, NET_PACKET_ALLOC_TAG);
			}
			NdisFreeMdl(CurrentMDL);
		}
		CurrentNetBuffer = NET_BUFFER_NEXT_NB(CurrentNetBuffer);
	}

	// Free the NBL in
	NdisFreeNetBufferList(CurrentNBL);
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

NDIS_STATUS WPFilterRestart(NDIS_HANDLE FilterModuleContext, NDIS_FILTER_RESTART_PARAMETERS* RestartParameters) {

	NDIS_STATUS status = NDIS_STATUS_SUCCESS;
	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)FilterModuleContext;


	NDIS_RESTART_ATTRIBUTES* ndisRestartAttributes = RestartParameters->RestartAttributes;
	PNDIS_RESTART_GENERAL_ATTRIBUTES GenAttr = NULL;

	status = NDIS_STATUS_SUCCESS;

	while (ndisRestartAttributes) {
		DbgPrint("%X\n", ndisRestartAttributes->Oid);
		if (ndisRestartAttributes->Oid == OID_GEN_MINIPORT_RESTART_ATTRIBUTES) {
			GenAttr = (PNDIS_RESTART_GENERAL_ATTRIBUTES)ndisRestartAttributes->Data;
			filterContext->MaxFrameSize = GenAttr->MtuSize + sizeof(ETH_HEADER);
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

NDIS_STATUS WPFilterPause(NDIS_HANDLE FilterModuleContext, NDIS_FILTER_PAUSE_PARAMETERS* pauseParameters) {

	DbgPrint("WPFilterPause\n");

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)(FilterModuleContext);
	NDIS_STATUS status;

	// Set the flag that the filter is going to pause
	NDIS_ACQUIRE_LOCK(&filterContext->FilterLock, FALSE);
	filterContext->FilterState = FilterPausing;
	NDIS_RELEASE_LOCK(&filterContext->FilterLock, FALSE);

	status = NDIS_STATUS_SUCCESS;

	filterContext->FilterState = FilterPaused;

	return status;
}

VOID WPFilterDetach(NDIS_HANDLE FilterModuleContext) {

	DbgPrint("WPFilterDetach\n");

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)FilterModuleContext;

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

NDIS_STATUS WPFilterSetModuleOptions(NDIS_HANDLE FilterModuleContext) {

	NDIS_STATUS status = NDIS_STATUS_SUCCESS;

	return status;
}

VOID WPFilterReceiveFromUpper(NDIS_HANDLE FilterModuleContext, PNET_BUFFER_LIST NetBufferLists, NDIS_PORT_NUMBER portNumber, ULONG sendFlags) {

	FILTER_CONTEXT* filterContext = (FILTER_CONTEXT*)FilterModuleContext;
	BOOLEAN dispatchLevel;

	for (NET_BUFFER_LIST* currNBL = NetBufferLists; currNBL != NULL; currNBL = NET_BUFFER_LIST_NEXT_NBL(currNBL)) {
		for (NET_BUFFER* currNB = NET_BUFFER_LIST_FIRST_NB(currNBL); currNB != NULL; currNB = NET_BUFFER_NEXT_NB(currNB)) {

			BYTE* PacketBuffer;
			ULONG PacketBufferLength;

			if (currNB->DataLength <= 0) {
				//It must not occurr
				continue;
			}
			PacketBufferLength = max(filterContext->MaxFrameSize, currNB->DataLength);
			PacketBuffer = ExAllocatePoolWithTag(NonPagedPoolNx, PacketBufferLength, NET_PACKET_ALLOC_TAG);
			if (PacketBuffer == NULL) {
				//Error occurr
				continue;
			}
			BYTE* ethDataPtr = NdisGetDataBuffer(currNB, currNB->DataLength, PacketBuffer, 1, 0);
			//If data in NB is contiguous, system will not auto copy the data, which means we should copy them manually
			if (ethDataPtr != (PacketBuffer)) {
				NdisMoveMemory((BYTE*)PacketBuffer, ethDataPtr, currNB->DataLength);
			}


			ExFreePoolWithTag(PacketBuffer, NET_PACKET_ALLOC_TAG);
		}
	}

	//NdisFSendNetBufferListsComplete(filterContext->FilterHandle, NetBufferLists, sendFlags);
	NdisFSendNetBufferLists(filterContext->FilterHandle, NetBufferLists, portNumber, sendFlags);

}

#define LinkSingleNBLIntoNBLChainTail(ChainHead,ChainTail,SingleNBLNode) {	\
	NET_BUFFER_LIST_NEXT_NBL(SingleNBLNode) = NULL;							\
	if (ChainHead == NULL || ChainTail == NULL) {							\
		ChainHead = SingleNBLNode;											\
		ChainTail = ChainHead;												\
	} else {																\
		NET_BUFFER_LIST_NEXT_NBL(ChainTail) = SingleNBLNode;				\
		ChainTail = NET_BUFFER_LIST_NEXT_NBL(ChainTail);					\
	}																		\
}

#define LinkSingleNBLIntoNBLChainHead(ChainHead,SingleNBLNode) {	\
	NET_BUFFER_LIST_NEXT_NBL(SingleNBLNode) = ChainHead;			\
	ChainHead = SingleNBLNode;										\
}

#define DropPacket(CanNotPend,NBLDropListHeader,SingleNBLPendingDrop,DropCounter,DroppedFlag) {	\
	if(!DroppedFlag){																			\
		if (!CanNotPend) {																		\
			LinkSingleNBLIntoNBLChainHead(NBLDropListHeader, SingleNBLPendingDrop);				\
		}																						\
		DropCounter++;																			\
		DroppedFlag = TRUE;																		\
	}																							\
}

VOID WPFilterReceiveFromNIC(NDIS_HANDLE FilterModuleContext, PNET_BUFFER_LIST NetBufferLists, NDIS_PORT_NUMBER PortNumber, ULONG NumberOfNetBufferLists, ULONG ReceiveFlags) {

	PFILTER_CONTEXT FilterContext = (PFILTER_CONTEXT)FilterModuleContext;
	BOOLEAN CanNotPend = NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags);
	BOOLEAN DispatchLevel = NDIS_TEST_RECEIVE_AT_DISPATCH_LEVEL(ReceiveFlags);;

	PNET_BUFFER_LIST AcceptListHead = NULL;
	PNET_BUFFER_LIST AcceptListTail = NULL;
	PNET_BUFFER_LIST ReturnList = NULL;
	ULONG AcceptListNBLCnt = 0;
	ULONG ReturnListNBLCnt = 0;

	PNET_BUFFER_LIST CurrentNBL = NetBufferLists;
	BYTE* PacketBuffer;
	ULONG PacketBufferLength;

	PacketBufferLength = FilterContext->MaxFrameSize;
	PacketBuffer = ExAllocatePoolWithTag(NonPagedPoolNx, PacketBufferLength, NET_PACKET_ALLOC_TAG);
	if (PacketBuffer == NULL) {
		//Error occurr. Return the NBL
		
		if (!CanNotPend) {
			NdisFReturnNetBufferLists(FilterContext->FilterHandle, NetBufferLists, ReceiveFlags);
		}
		// if (CanNotPend) :
		// Just return from this function to drop packets without calling NdisFReturnNetBufferLists.
		return;
	}


	while (CurrentNBL != NULL) {
		// Note: on the receive path for Ethernet packets, one NBL will have exactly one NB. 
		// So you do not need to worry about dropping one NB, but trying to indicate up the remaining NBs on the same NBL.
		// In other words, if the first NB should be dropped, drop the whole NBL.
		PNET_BUFFER CurrentNB = NET_BUFFER_LIST_FIRST_NB(CurrentNBL);

		PNET_BUFFER_LIST TempNBL = NET_BUFFER_LIST_NEXT_NBL(CurrentNBL);
		BYTE* EthDataPtr;
		HOOK_RESULT PreroutingFilterResult;
		HOOK_RESULT InputFilterResult;
		ULONG DataLength = CurrentNB->DataLength;
		BOOLEAN Dropped = FALSE;
		PNET_BUFFER_LIST IndicateNBL;
		BYTE* IndicatePacketBuffer = NULL;
		PMDL IndicatePacketBufferMDL = NULL;

		do {
			if (DataLength == 0 || DataLength > PacketBufferLength) {
				//It must not occurr. Return the NBL.
				DropPacket(CanNotPend, ReturnList, CurrentNBL, ReturnListNBLCnt,Dropped);
				break;
			}

			RtlZeroMemory(PacketBuffer, PacketBufferLength);
			EthDataPtr = NdisGetDataBuffer(CurrentNB, DataLength, PacketBuffer, 1, 0);
			//If data in NB is contiguous, system will not auto copy the data, which means we should copy them manually
			if (EthDataPtr != (PacketBuffer)) {
				RtlCopyMemory((BYTE*)PacketBuffer, EthDataPtr, DataLength);
			}


			/*========== THE PRE_ROUTING FILTER POINT ==========*/
			PreroutingFilterResult = FilterEthernetPacket(
						PacketBuffer, 
						&DataLength, 
						PacketBufferLength,
						FILTER_POINT_PREROUTING,
						FilterContext->MiniportIfIndex,
						DispatchLevel
					);
			
			if (!PreroutingFilterResult.Accept) {
				// Drop this packet
				DropPacket(CanNotPend, ReturnList, CurrentNBL, ReturnListNBLCnt, Dropped);

				// If not modified, go next packet, otherwise continue process this one
				if (!PreroutingFilterResult.Modified) {
					break;
				}
			}
			/*========== END PRE_ROUTING FILTER POINT ==========*/




			// TODO: routing decision




			/*========== THE INPUT FILTER POINT ==========*/
			InputFilterResult = FilterEthernetPacket(
				PacketBuffer,
				&DataLength,
				PacketBufferLength,
				FILTER_POINT_INPUT,
				FilterContext->MiniportIfIndex,
				DispatchLevel
			);

			if (!InputFilterResult.Accept) {
				// Drop this packet
				DropPacket(CanNotPend, ReturnList, CurrentNBL, ReturnListNBLCnt, Dropped);

				// If not modified, go next packet, otherwise continue process this one
				if (!InputFilterResult.Modified) {
					break;
				}
			}
			/*========== END INPUT FILTER POINT ==========*/

			IndicateNBL = CurrentNBL;

			if (PreroutingFilterResult.Modified || InputFilterResult.Modified) {
				// The hook function modified the buffer
				// We need assign a new NBl for the Packet
				// First, create a new buffer 
				IndicatePacketBuffer = ExAllocatePoolWithTag(NonPagedPoolNx, DataLength, NET_PACKET_ALLOC_TAG);
				if (IndicatePacketBuffer == NULL) {
					// Error occurred! Drop this packet
					DropPacket(CanNotPend, ReturnList, CurrentNBL, ReturnListNBLCnt, Dropped);
					break;
				}
				// Second, Allocate an MDL for the buffer and copy the data
				IndicatePacketBufferMDL = NdisAllocateMdl(FilterContext->FilterHandle, IndicatePacketBuffer, DataLength);
				if (IndicatePacketBufferMDL == NULL) {
					ExFreePoolWithTag(IndicatePacketBuffer, NET_PACKET_ALLOC_TAG);
					// Error occurred! Drop this packet
					DropPacket(CanNotPend, ReturnList, CurrentNBL, ReturnListNBLCnt, Dropped);
					break;
				}
				RtlZeroMemory(IndicatePacketBuffer, DataLength);
				RtlCopyMemory(IndicatePacketBuffer, PacketBuffer, DataLength);
				// Finally, allocate NBL and NB.
				IndicateNBL = NdisAllocateNetBufferAndNetBufferList(FilterContext->NBLPool, 0, 0, IndicatePacketBufferMDL, 0, DataLength);
				if (IndicateNBL == NULL) {
					// Error occurred! Drop this packet
					NdisFreeMdl(IndicatePacketBufferMDL);
					ExFreePoolWithTag(IndicatePacketBuffer, NET_PACKET_ALLOC_TAG);
					DropPacket(CanNotPend, ReturnList, CurrentNBL, ReturnListNBLCnt, Dropped);
					break;
				}

				// Set properties of new NBL
				IndicateNBL->SourceHandle = FilterContext->FilterHandle;
				NdisCopyReceiveNetBufferListInfo(IndicateNBL, CurrentNBL);

			}


			// Indicate up this packet
			AcceptListNBLCnt++;
			if (!CanNotPend) {
				LinkSingleNBLIntoNBLChainTail(AcceptListHead, AcceptListTail, IndicateNBL);
				break;
			}

			// if (CanNotPend) :
			// For each NBL that is NOT dropped, temporarily unlink it from the linked list.
			NET_BUFFER_LIST_NEXT_NBL(IndicateNBL) = NULL;
			// Indicate it up alone with NdisFIndicateReceiveNetBufferLists and the NDIS_RECEIVE_FLAGS_RESOURCES flag set.
			NdisFIndicateReceiveNetBufferLists(FilterContext->FilterHandle, IndicateNBL, PortNumber,
				1, ReceiveFlags | NDIS_RECEIVE_FLAGS_RESOURCES);

			// IndicateNBL was not been created by us.
			if (IndicateNBL->SourceHandle != FilterContext->FilterHandle) {
				// Then immediately relink the NBL back into the chain.
				NET_BUFFER_LIST_NEXT_NBL(IndicateNBL) = TempNBL;
				break;
			}
			// Uur NBL, Free it!
			FreeSingleNBL(IndicateNBL);

		} while (FALSE);

		CurrentNBL = TempNBL;
	}

	ExFreePoolWithTag(PacketBuffer, NET_PACKET_ALLOC_TAG);

	TRACE_DBG("CanNotPend:%d\n", CanNotPend);
	TRACE_DBG("TotalCnt:%d\nReturnListCnt:%d\nAcceptListCnt:%d\n", NumberOfNetBufferLists, ReturnListNBLCnt, AcceptListNBLCnt);

	// if (CanNotPend) :
	// When all NBLs have been indicated up , return from this function.
	if (CanNotPend) {
		return;
	}

	if (ReturnList != NULL) {
		// Drop the packtes in ReturnList.
		NdisFReturnNetBufferLists(FilterContext->FilterHandle, ReturnList, ReceiveFlags);
	}
	if (AcceptListHead != NULL) {
		// Accept the packtes in AcceptList.
		NdisFIndicateReceiveNetBufferLists(FilterContext->FilterHandle, AcceptListHead, PortNumber, AcceptListNBLCnt, ReceiveFlags);
	}

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
