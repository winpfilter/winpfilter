#pragma once
#include "winpfilter.h"

#define ROUTING_PROC_COUNT 6


typedef ULONG ROUTING_PROC;

#define ROUTING_PROC_PREROUTING	 0
#define ROUTING_PROC_INPUT		 1
#define ROUTING_PROC_FORWARDING	 2
#define ROUTING_PROC_OUTPUT		 3
#define ROUTING_PROC_POSTROUTING 4
#define ROUTING_PROC_RTDECISION 5

typedef struct _ROUTING_BUFFER {
	BYTE* Buffer;
	ULONG DataLength;
	ULONG BufferLength;
	BOOLEAN IsModified;
}ROUTING_BUFFER, * PROUTING_BUFFER;

typedef struct _ROUTING_TASK {
	ROUTING_BUFFER DataBuffer;
	// Only include offload info
	PVOID NetBufferListInfo[5];
	PFILTER_CONTEXT FilterContext;
	LIST_ENTRY QueueNode;
}ROUTING_TASK,*PROUTING_TASK;


VOID FreeRoutingTask(PROUTING_TASK Task);
VOID FreeRoutingTaskWithoutDataBuffer(PROUTING_TASK Task);
VOID CreateAndProcessRoutingTask(BYTE* EthernetBuffer, ULONG DataLength, ROUTING_PROC FilterPoint, PFILTER_CONTEXT FilterContext, PNET_BUFFER_LIST ReferenceNBL);
NTSTATUS CreateAndProcessRoutingTaskWithoutDataBufferCopy(BYTE* Buffer, ULONG DataLength, ULONG BufferLength, ROUTING_PROC RoutingPos, PFILTER_CONTEXT FilterContext, PNET_BUFFER_LIST ReferenceNBL);

VOID StartRoutingEngine();
VOID StopRoutingEngine();