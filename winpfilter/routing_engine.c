#include "routing_engine.h"
#include "queue.h"
#include "global_variables.h"
#include "ether.h"
#include "route.h"
#include "hook_manager.h"

QUEUE TaskQueues[ROUTING_PROC_COUNT];
PKTHREAD RoutingThreads[ROUTING_PROC_COUNT];
KEVENT StopSignal;
BOOLEAN ThreadFlag;

#define WaitCount (sizeof(WaitList)/sizeof(PVOID))

inline VOID ProcessRoutingTask(PROUTING_TASK Task, ROUTING_PROC RoutingPos) {
	Enqueue(&TaskQueues[RoutingPos], &(Task->QueueNode));
}


VOID PreroutingThread() {
	TRACE_DBG("PreroutingThread Start\n");
	PVOID WaitList[] = { &StopSignal ,&(TaskQueues[ROUTING_PROC_PREROUTING].DataEnqueueEvent) };
	BYTE WaitBuffer[sizeof(KWAIT_BLOCK) * WaitCount];
	while (ThreadFlag)
	{
		KeWaitForMultipleObjects(WaitCount,WaitList, WaitAny, Executive, KernelMode, FALSE, NULL, (PKWAIT_BLOCK)WaitBuffer);
		TRACE_DBG("Processing Prerouting Task\n");
		PLIST_ENTRY ProcessingTaskListNode = Dequeue(&TaskQueues[ROUTING_PROC_PREROUTING]);
		if (ProcessingTaskListNode == NULL) {
			// Error or stop signal
			continue;
		}

		PROUTING_TASK Task = CONTAINING_RECORD(ProcessingTaskListNode, ROUTING_TASK, QueueNode);

		/*========== THE PRE_ROUTING FILTER POINT ==========*/
		HOOK_RESULT PreroutingFilterResult = FilterEthernetPacket(
			Task->DataBuffer.Buffer,
			&Task->DataBuffer.DataLength,
			Task->DataBuffer.BufferLength,
			FILTER_POINT_PREROUTING,
			Task->FilterContext->BasePortLuid,
			0
		);
		/*========== END PRE_ROUTING FILTER POINT ==========*/

		if (!PreroutingFilterResult.Accept) {
			// Drop this packet
			// If not modified, go next packet, otherwise continue process this one
			FreeRoutingTask(Task);
			continue;
		}

		if (PreroutingFilterResult.Modified) {
			Task->DataBuffer.IsModified = TRUE;
		}
		// Packed accepted or modified

		if (IPForwardingMode != IP_FORWARDING_MODE_SYSTEM) {
			// determining packet forwarding 
			PETH_HEADER EtherHeader = (PETH_HEADER)Task->DataBuffer.Buffer;
			USHORT EtherProtocol = GetEtherHeaderProtocol(EtherHeader);
			BYTE* IpHeader = GetNetworkLayerHeaderFromEtherHeader(EtherHeader);
			BYTE* DestIpAddressBytes;

			BYTE ForwardingFlag = FALSE;

			if (EtherProtocol == ETH_PROTOCOL_IP || EtherProtocol == ETH_PROTOCOL_IPV6) {
				if (EtherProtocol == ETH_PROTOCOL_IP) {
					// IPv4
					DestIpAddressBytes = ((PIPV4_HEADER)IpHeader)->DestAddress.AddressBytes;
				}
				else {
					// IPv6
					DestIpAddressBytes = ((PIPV6_HEADER)IpHeader)->DestAddress.AddressBytes;
				}
				ForwardingFlag = IsValidForwardAddress(EtherProtocol, Task->FilterContext->BasePortLuid, DestIpAddressBytes);
			}

			if (ForwardingFlag == TRUE) {
				//The IP packet is not for local machine
				if (IPForwardingMode == IP_FORWARDING_MODE_WINPFILTER) {
					// Join forwarding queue
					ProcessRoutingTask(Task, ROUTING_PROC_FORWARDING);
				}
				else if (IPForwardingMode == IP_FORWARDING_MODE_DISABLE) {
					FreeRoutingTask(Task);
				}
				continue;
			}

		}

		ProcessRoutingTask(Task, ROUTING_PROC_INPUT);
		continue;
	}
	TRACE_DBG("PreroutingThread Stop\n");
	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID InputThread() {
	TRACE_DBG("InputThread Start\n");
	PVOID WaitList[] = { &StopSignal ,&(TaskQueues[ROUTING_PROC_INPUT].DataEnqueueEvent) };
	BYTE WaitBuffer[sizeof(KWAIT_BLOCK) * WaitCount];
	while (ThreadFlag)
	{
		KeWaitForMultipleObjects(WaitCount, WaitList, WaitAny, Executive, KernelMode, FALSE, NULL, (PKWAIT_BLOCK)WaitBuffer);
		TRACE_DBG("Processing Input Task\n");
		PLIST_ENTRY ProcessingTaskListNode = Dequeue(&TaskQueues[ROUTING_PROC_INPUT]);
		if (ProcessingTaskListNode == NULL) {
			// Error or stop signal
			continue;
		}

		PROUTING_TASK Task = CONTAINING_RECORD(ProcessingTaskListNode, ROUTING_TASK, QueueNode);

		/*========== THE INPUT FILTER POINT ==========*/
		HOOK_RESULT InputFilterResult = FilterEthernetPacket(
			Task->DataBuffer.Buffer,
			&Task->DataBuffer.DataLength,
			Task->DataBuffer.BufferLength,
			FILTER_POINT_INPUT,
			Task->FilterContext->BasePortLuid,
			0
		);
		/*========== END INPUT FILTER POINT ==========*/

		if (!InputFilterResult.Accept) {
			// Drop this packet
			// If not modified, go next packet, otherwise continue process this one
			FreeRoutingTask(Task);
			continue;
		}
		if (InputFilterResult.Modified) {
			Task->DataBuffer.IsModified = TRUE;
		}
		// We need assign a new NBl for the Packet
		// First, use a old buffer 
		// Second, Allocate an MDL for the buffer and copy the data
		PMDL IndicatePacketBufferMDL = NdisAllocateMdl(Task->FilterContext->FilterHandle, Task->DataBuffer.Buffer, Task->DataBuffer.DataLength);
		if (IndicatePacketBufferMDL == NULL) {
			// Error occurred! Drop this packet
			FreeRoutingTask(Task);
			continue;
		}
		// Finally, allocate NBL and NB.
		PNET_BUFFER_LIST IndicateNBL = NdisAllocateNetBufferAndNetBufferList(Task->FilterContext->NBLPool, 0, 0, IndicatePacketBufferMDL, 0, Task->DataBuffer.DataLength);
		if (IndicateNBL == NULL) {
			// Error occurred! Drop this packet
			NdisFreeMdl(IndicatePacketBufferMDL);
			FreeRoutingTask(Task);
			continue;
		}

		// Set properties of new NBL
		IndicateNBL->SourceHandle = Task->FilterContext->FilterHandle;
		RtlCopyMemory(IndicateNBL->NetBufferListInfo,Task->NetBufferListInfo , sizeof(Task->NetBufferListInfo));

		if (Task->DataBuffer.IsModified) {
			NDIS_TCP_IP_CHECKSUM_NET_BUFFER_LIST_INFO ChecksumOffloadInfo;
			ChecksumOffloadInfo.Value = 0;
			ChecksumOffloadInfo.Receive.IpChecksumSucceeded = IndicateModifiedRxPacketWithBadIPChecksumAsGood;
			ChecksumOffloadInfo.Receive.TcpChecksumSucceeded = IndicateModifiedRxPacketWithBadTCPChecksumAsGood;
			ChecksumOffloadInfo.Receive.UdpChecksumSucceeded = IndicateModifiedRxPacketWithBadUDPChecksumAsGood;
			IndicateNBL->NetBufferListInfo[TcpIpChecksumNetBufferListInfo] = ChecksumOffloadInfo.Value;
		}

		NdisFIndicateReceiveNetBufferLists(Task->FilterContext->FilterHandle, IndicateNBL, NDIS_DEFAULT_PORT_NUMBER, 1, 0);
		FreeRoutingTaskWithoutDataBuffer(Task);

	}
	TRACE_DBG("InputThread Stop\n");
	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID ForwardingThread() {
	TRACE_DBG("ForwardingThread Start\n");
	PVOID WaitList[] = { &StopSignal ,&(TaskQueues[ROUTING_PROC_FORWARDING].DataEnqueueEvent	) };
	BYTE WaitBuffer[sizeof(KWAIT_BLOCK) * WaitCount];
	while (ThreadFlag)
	{
		KeWaitForMultipleObjects(WaitCount, WaitList, WaitAny, Executive, KernelMode, FALSE, NULL, (PKWAIT_BLOCK)WaitBuffer);
		TRACE_DBG("Processing Forwarding Task\n");
		PLIST_ENTRY ProcessingTaskListNode = Dequeue(&TaskQueues[ROUTING_PROC_FORWARDING]);
		if (ProcessingTaskListNode == NULL) {
			// Error or stop signal
			continue;
		}

		PROUTING_TASK Task = CONTAINING_RECORD(ProcessingTaskListNode, ROUTING_TASK, QueueNode);

		/*========== THE FORWARDING FILTER POINT ==========*/
		HOOK_RESULT ForwardingFilterResult = FilterEthernetPacket(
			Task->DataBuffer.Buffer,
			&Task->DataBuffer.DataLength,
			Task->DataBuffer.BufferLength,
			FILTER_POINT_FORWARDING,
			Task->FilterContext->BasePortLuid,
			0
		);
		/*========== END FORWARDING FILTER POINT ==========*/

		if (!ForwardingFilterResult.Accept) {
			// Drop this packet
			// If not modified, go next packet, otherwise continue process this one
			FreeRoutingTask(Task);
			continue;
		}

		if (ForwardingFilterResult.Modified) {
			Task->DataBuffer.IsModified = TRUE;
		}

		ProcessRoutingTask(Task, ROUTING_PROC_RTDECISION);
		continue;
	}
	TRACE_DBG("ForwardingThread Stop\n");
	PsTerminateSystemThread(STATUS_SUCCESS);
}


VOID OutputThread() {
	TRACE_DBG("OutputThread Start\n");
	PVOID WaitList[] = { &StopSignal ,&(TaskQueues[ROUTING_PROC_OUTPUT].DataEnqueueEvent) };
	BYTE WaitBuffer[sizeof(KWAIT_BLOCK) * WaitCount];
	while (ThreadFlag)
	{
		KeWaitForMultipleObjects(WaitCount, WaitList, WaitAny, Executive, KernelMode, FALSE, NULL, (PKWAIT_BLOCK)WaitBuffer);
		TRACE_DBG("Processing Output Task\n");
		PLIST_ENTRY ProcessingTaskListNode = Dequeue(&TaskQueues[ROUTING_PROC_OUTPUT]);
		if (ProcessingTaskListNode == NULL) {
			// Error or stop signal
			continue;
		}

		PROUTING_TASK Task = CONTAINING_RECORD(ProcessingTaskListNode, ROUTING_TASK, QueueNode);


		/*========== THE OUTPUT FILTER POINT ==========*/
		HOOK_RESULT OutputFilterResult = FilterEthernetPacket(
			Task->DataBuffer.Buffer,
			&Task->DataBuffer.DataLength,
			Task->DataBuffer.BufferLength,
			FILTER_POINT_OUTPUT,
			Task->FilterContext->BasePortLuid,
			0
		);
		/*========== END OUTPUT FILTER POINT ==========*/

		if (!OutputFilterResult.Accept) {
			// Modified = FALSE 
			// Accept = FALSE
			// Drop the packet
			FreeRoutingTask(Task);
			continue;
		}

		if (OutputFilterResult.Modified) {
			// Modified = TRUE 
			// Accept = TRUE
			//
			// Join the routing decision queue
			Task->DataBuffer.IsModified = TRUE;
			ProcessRoutingTask(Task, ROUTING_PROC_RTDECISION);
			continue;
		}

		// else
		// Modified = FALSE 
		// Accept = TRUE
		// Do postrouting here
		ProcessRoutingTask(Task, ROUTING_PROC_POSTROUTING);
		continue;

	}
	TRACE_DBG("OutputThread Stop\n");
	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID PostroutingThread() {
	TRACE_DBG("PostroutingThread Start\n");
	PVOID WaitList[] = { &StopSignal ,&(TaskQueues[ROUTING_PROC_POSTROUTING].DataEnqueueEvent) };
	BYTE WaitBuffer[sizeof(KWAIT_BLOCK) * WaitCount];
	while (ThreadFlag)
	{
		KeWaitForMultipleObjects(WaitCount, WaitList, WaitAny, Executive, KernelMode, FALSE, NULL, (PKWAIT_BLOCK)WaitBuffer);
		TRACE_DBG("Processing Postrouting Task\n");
		PLIST_ENTRY ProcessingTaskListNode = Dequeue(&TaskQueues[ROUTING_PROC_POSTROUTING]);
		if (ProcessingTaskListNode == NULL) {
			// Error or stop signal
			continue;
		}

		PROUTING_TASK Task = CONTAINING_RECORD(ProcessingTaskListNode, ROUTING_TASK, QueueNode);

		/*========== THE POSTROUTING FILTER POINT ==========*/
		HOOK_RESULT PostroutingFilterResult = FilterEthernetPacket(
			Task->DataBuffer.Buffer,
			&Task->DataBuffer.DataLength,
			Task->DataBuffer.BufferLength,
			FILTER_POINT_POSTROUTING,
			Task->FilterContext->BasePortLuid,
			0
		);
		/*========== END POSTROUTING FILTER POINT ==========*/

		if (!PostroutingFilterResult.Accept) {
			// Drop this packet
			// If not modified, go next packet, otherwise continue process this one
			FreeRoutingTask(Task);
			continue;
		}

		if (PostroutingFilterResult.Modified) {
			Task->DataBuffer.IsModified = TRUE;
		}

		// We need assign a new NBl for the Packet
		// First, use a old buffer 
		// Second, Allocate an MDL for the buffer and copy the data
		PMDL PostroutingPacketBufferMDL = NdisAllocateMdl(Task->FilterContext->FilterHandle, Task->DataBuffer.Buffer, Task->DataBuffer.DataLength);
		if (PostroutingPacketBufferMDL == NULL) {
			// Error occurred! Drop this packet
			FreeRoutingTask(Task);
			continue;
		}
		// Finally, allocate NBL and NB.
		PNET_BUFFER_LIST PostroutingNBL = NdisAllocateNetBufferAndNetBufferList(Task->FilterContext->NBLPool, 0, 0, PostroutingPacketBufferMDL, 0, Task->DataBuffer.DataLength);
		if (PostroutingNBL == NULL) {
			// Error occurred! Drop this packet
			NdisFreeMdl(PostroutingPacketBufferMDL);
			FreeRoutingTask(Task);
			continue;
		}

		// Set properties of new NBL
		PostroutingNBL->SourceHandle = Task->FilterContext->FilterHandle;
		RtlCopyMemory(PostroutingNBL->NetBufferListInfo, Task->NetBufferListInfo, sizeof(Task->NetBufferListInfo));


		if (Task->DataBuffer.IsModified) {
			PETH_HEADER PacketEthHeader = (PETH_HEADER)Task->DataBuffer.Buffer;
			NDIS_TCP_IP_CHECKSUM_NET_BUFFER_LIST_INFO ChecksumOffloadInfo;
			ChecksumOffloadInfo.Value = 0;
			ChecksumOffloadInfo.Transmit.IsIPv4 = (GetEtherHeaderProtocol(PacketEthHeader) == ETH_PROTOCOL_IP);
			ChecksumOffloadInfo.Transmit.IsIPv6 = (GetEtherHeaderProtocol(PacketEthHeader) == ETH_PROTOCOL_IPV6);
			if (ChecksumOffloadInfo.Transmit.IsIPv4) {
				ChecksumOffloadInfo.Transmit.IpHeaderChecksum = (ChecksumOffloadInfo.Transmit.IsIPv4) & TryCalcModifiedTxPacketIPChecksumByNIC & Task->FilterContext->OffloadConfig->Checksum.IPv4Transmit.IpChecksum;
				ChecksumOffloadInfo.Transmit.TcpChecksum = (GetIPv4HeaderProtocol((PIPV4_HEADER)GetNetworkLayerHeaderFromEtherHeader(PacketEthHeader)) == TCP) & Task->FilterContext->OffloadConfig->Checksum.IPv4Transmit.TcpChecksum;
				ChecksumOffloadInfo.Transmit.UdpChecksum = (GetIPv4HeaderProtocol((PIPV4_HEADER)GetNetworkLayerHeaderFromEtherHeader(PacketEthHeader)) == UDP) & Task->FilterContext->OffloadConfig->Checksum.IPv4Transmit.UdpChecksum;
			}
			else if (ChecksumOffloadInfo.Transmit.IsIPv6) {
				ChecksumOffloadInfo.Transmit.IpHeaderChecksum = (ChecksumOffloadInfo.Transmit.IsIPv4) & TryCalcModifiedTxPacketIPChecksumByNIC ;
				ChecksumOffloadInfo.Transmit.TcpChecksum = (GetTransportLayerProtocolFromIPv6Header((PIPV6_HEADER)GetNetworkLayerHeaderFromEtherHeader(PacketEthHeader)) == TCP) & Task->FilterContext->OffloadConfig->Checksum.IPv6Transmit.TcpChecksum;
				ChecksumOffloadInfo.Transmit.UdpChecksum = (GetTransportLayerProtocolFromIPv6Header((PIPV6_HEADER)GetNetworkLayerHeaderFromEtherHeader(PacketEthHeader)) == UDP) & Task->FilterContext->OffloadConfig->Checksum.IPv6Transmit.UdpChecksum;
			}

			ChecksumOffloadInfo.Transmit.TcpChecksum &= TryCalcModifiedTxPacketTCPChecksumByNIC;
			ChecksumOffloadInfo.Transmit.UdpChecksum &= TryCalcModifiedTxPacketUDPChecksumByNIC;

			if (ChecksumOffloadInfo.Transmit.TcpChecksum) {
				if (ChecksumOffloadInfo.Transmit.IsIPv4) {
					ChecksumOffloadInfo.Transmit.TcpHeaderOffset =(ULONG)((ULONGLONG)GetTransportLayerHeaderFromIPv4Header(GetNetworkLayerHeaderFromEtherHeader(PacketEthHeader)) - (ULONGLONG)PacketEthHeader);
				}
				else {
					ChecksumOffloadInfo.Transmit.TcpHeaderOffset = (ULONG)((ULONGLONG)GetTransportLayerHeaderFromIPv6Header(GetNetworkLayerHeaderFromEtherHeader(PacketEthHeader)) - (ULONGLONG)PacketEthHeader);
				}
			}

			PostroutingNBL->NetBufferListInfo[TcpIpChecksumNetBufferListInfo] = ChecksumOffloadInfo.Value;
		}

		TRACE_DBG("Processing NBL Send\n");
		NdisFSendNetBufferLists(Task->FilterContext->FilterHandle, PostroutingNBL, NDIS_DEFAULT_PORT_NUMBER, 0);
		FreeRoutingTaskWithoutDataBuffer(Task);
		continue;
	}
	TRACE_DBG("PostroutingThread Stop\n");
	PsTerminateSystemThread(STATUS_SUCCESS);
}


VOID RoutingDecisionThread() {
	TRACE_DBG("RoutingDecisionThread Start\n");
	PVOID WaitList[] = { &StopSignal ,&(TaskQueues[ROUTING_PROC_RTDECISION].DataEnqueueEvent) };
	BYTE WaitBuffer[sizeof(KWAIT_BLOCK) * WaitCount];
	while (ThreadFlag)
	{
		KeWaitForMultipleObjects(WaitCount, WaitList, WaitAny, Executive, KernelMode, FALSE, NULL, (PKWAIT_BLOCK)WaitBuffer);
		TRACE_DBG("Processing RoutingDecision Task\n");
		PLIST_ENTRY ProcessingTaskListNode = Dequeue(&TaskQueues[ROUTING_PROC_RTDECISION]);
		if (ProcessingTaskListNode == NULL) {
			// Error or stop signal
			continue;
		}

		PROUTING_TASK Task = CONTAINING_RECORD(ProcessingTaskListNode, ROUTING_TASK, QueueNode);


		//TODO
	}
	TRACE_DBG("RoutingDecisionThread Stop\n");
	PsTerminateSystemThread(STATUS_SUCCESS);
}




VOID StartRoutingEngine() {
	ThreadFlag = TRUE;
	KeInitializeEvent(&StopSignal, NotificationEvent, FALSE);

	PKSTART_ROUTINE RoutingThreadRoutines[] = { 
		(PKSTART_ROUTINE)PreroutingThread,
		(PKSTART_ROUTINE)InputThread,
		(PKSTART_ROUTINE)ForwardingThread ,
		(PKSTART_ROUTINE)OutputThread,
		(PKSTART_ROUTINE)PostroutingThread,
		(PKSTART_ROUTINE)RoutingDecisionThread
	};
	HANDLE ThreadHandle;
	for (int i = 0; i < ROUTING_PROC_COUNT; i++) {
		PsCreateSystemThread(&ThreadHandle, THREAD_ALL_ACCESS, NULL, NULL, NULL, RoutingThreadRoutines[i], NULL);
		ObReferenceObjectByHandle(ThreadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, &RoutingThreads[i], NULL);
		ZwClose(ThreadHandle);
		InitializeQueue(&(TaskQueues[i]));
	}
}

VOID StopRoutingEngine() {
	ThreadFlag = FALSE;
	KeSetEvent(&StopSignal, IO_NO_INCREMENT, FALSE); 
	BYTE WaitBuffer[sizeof(KWAIT_BLOCK)* ROUTING_PROC_COUNT];
	KeWaitForMultipleObjects(ROUTING_PROC_COUNT, (PVOID*) ( & RoutingThreads), WaitAll, Executive, KernelMode, FALSE, NULL, (PKWAIT_BLOCK)WaitBuffer);
	for (int i = 0; i < ROUTING_PROC_COUNT; i++) {
		ObDereferenceObject(RoutingThreads[i]);
		NdisAcquireSpinLock(&TaskQueues[i].QueueLock);
		PLIST_ENTRY TaskPtr;
		while (
				(TaskPtr = RemoveHeadList(&TaskQueues[i].QueueLink)) 
				!= &(TaskQueues[i].QueueLink)
			) {
			FreeRoutingTask(CONTAINING_RECORD(TaskPtr, ROUTING_TASK, QueueNode));
		}
		NdisReleaseSpinLock(&TaskQueues[i].QueueLock);

		FreeQueue(&TaskQueues[i]);
	}
}

VOID CreateAndProcessRoutingTask(BYTE* EthernetBuffer, ULONG DataLength, ROUTING_PROC RoutingPos, PFILTER_CONTEXT FilterContext, PNET_BUFFER_LIST ReferenceNBL) {
	PROUTING_TASK Task = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(ROUTING_TASK), ROUTING_TASK_ALLOC_TAG);
	if (Task == NULL) {
		return;
	}
	RtlZeroMemory(Task, sizeof(ROUTING_TASK));
	Task->DataBuffer.Buffer = ExAllocatePoolWithTag(NonPagedPoolNx, DataLength, ROUTING_TASK_ALLOC_TAG);
	if (Task->DataBuffer.Buffer == NULL) {
		FreeRoutingTask(Task);
		return;
	}
	RtlZeroMemory(Task->DataBuffer.Buffer, DataLength);
	Task->DataBuffer.DataLength = DataLength;
	Task->DataBuffer.BufferLength = DataLength;
	RtlCopyMemory(Task->DataBuffer.Buffer, EthernetBuffer, Task->DataBuffer.DataLength);
	Task->FilterContext = FilterContext;
	RtlCopyMemory(Task->NetBufferListInfo, ReferenceNBL->NetBufferListInfo, sizeof(Task->NetBufferListInfo));
	ProcessRoutingTask(Task, RoutingPos);
}

NTSTATUS CreateAndProcessRoutingTaskWithoutDataBufferCopy(BYTE* Buffer, ULONG DataLength, ULONG BufferLength, ROUTING_PROC RoutingPos, PFILTER_CONTEXT FilterContext, PNET_BUFFER_LIST ReferenceNBL) {
	PROUTING_TASK Task = ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(ROUTING_TASK), ROUTING_TASK_ALLOC_TAG);
	if (Task == NULL) {
		return STATUS_UNSUCCESSFUL;
	}
	RtlZeroMemory(Task, sizeof(ROUTING_TASK));
	Task->DataBuffer.Buffer = Buffer;
	Task->DataBuffer.DataLength = DataLength;
	Task->DataBuffer.BufferLength = BufferLength;
	Task->FilterContext = FilterContext;
	RtlCopyMemory(Task->NetBufferListInfo, ReferenceNBL->NetBufferListInfo, sizeof(Task->NetBufferListInfo));
	ProcessRoutingTask(Task, RoutingPos);
	return STATUS_SUCCESS;
}

VOID FreeRoutingTaskWithoutDataBuffer(PROUTING_TASK Task) {
	if (Task != NULL) {
		ExFreePoolWithTag(Task, ROUTING_TASK_ALLOC_TAG);
		Task = NULL;
	}
}

VOID FreeRoutingTask(PROUTING_TASK Task) {
	if (Task != NULL) {
		if (Task->DataBuffer.Buffer != NULL) {
			ExFreePool(Task->DataBuffer.Buffer);
			Task->DataBuffer.Buffer = NULL;
		}
		ExFreePoolWithTag(Task, ROUTING_TASK_ALLOC_TAG);
		Task = NULL;
	}
}