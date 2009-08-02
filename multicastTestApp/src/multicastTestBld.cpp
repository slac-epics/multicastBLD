#include <stdio.h>
#include <unistd.h>
#include <malloc.h>

#ifdef __rtems__
#include <rtems.h>
#endif

#include "multicastBLDLib.h"
#include "multicastTestBasic.h"

extern "C" 
{
// forward declarations
int testBldAPI_C(char* sInterfaceIp); 
int testBldAPI_CPP(char* sInterfaceIp); 

/*
 * rtems utility functions
 */
 
/**
 * rtems thread clean up function
 *
 * Only to be used with rtems_gdb_thread_helper
 * NOT to be used in CExp shell
 */
//inline void rtemsThreadExit()
//{
//#ifdef __rtems__
//	rtems_task_delete(RTEMS_SELF);
//#endif	
//}

//inline void rtemsBreakpoint()
//{
//#ifdef __rtems__
//	asm volatile("sc");
//#endif	
//}

int testBldClient(int iTestType, char* sInterfaceIp)
{	
	if ( iTestType == 0 )
	{	
		testBldAPI_CPP(sInterfaceIp);
	}
	else if ( iTestType == 1 )
	{
		testBldAPI_C(sInterfaceIp); 
	}
	else if ( iTestType == 2 )
	{
		testMulticastSample(sInterfaceIp);
	}
	
	return 0;
}

int testBldAPI_CPP(char* sInterfaceIp)
{	
	const unsigned int uAddr = 239<<24 | 255<<16 | 0<<8 | 1; // multicast address	
	//const unsigned int uAddr = 172<<24 | 21<<16 | 9<<8 | 21; // multicast address
	const unsigned int uPort = 50000;
	const unsigned int uMaxDataSize = 256; // in bytes
	const unsigned char ucTTL = 32; /// minimum: 1 + (# of routers in the middle)	
	
	const char lcData[] = "MULTICAST BLD TEST"; // The sizeof(lcData) < uMaxDataSize
		
	EpicsBld::BldClientInterface* pBldClient = 
	  EpicsBld::BldClientFactory::createBldClient(uAddr, uPort, uMaxDataSize, 
	  ucTTL, sInterfaceIp);			
	  	  
	printf("Bld send to %x port %d Data String: %s\n", uAddr, uPort, lcData);
	int iFailSend = pBldClient->sendRawData(sizeof(lcData), lcData);
		
	delete pBldClient;
	
	/*
	 * Error Report
	 */
	if ( iFailSend != 0 )
		printf( "[Error] testBldAPI_CPP() : pBldClient->sendRawData() failed\n" );
	
	return 0;
}

int testBldAPI_C(char* sInterfaceIp)
{
	const unsigned int uAddr = 239<<24 | 255<<16 | 0<<8 | 1; // multicast address
	const unsigned int uPort = 50000;
	const unsigned int uMaxDataSize = 256; // in bytes
	const unsigned char ucTTL = 32; /// minimum: 1 + (# of routers in the middle)
	
	void* pVoidBldClient = NULL;
	BldClientInitByInterfaceName(uAddr, uPort, uMaxDataSize, ucTTL, sInterfaceIp, 
	  &pVoidBldClient);
	
	if ( pVoidBldClient == NULL ) return 1;
	
	printf( "Beginning Multicast Client Testing...\n" );
	
	// Allocate data buffer
	unsigned int uIntDataSize = (uMaxDataSize/sizeof(int));
	int* liData = (int*) malloc(uIntDataSize * sizeof(int));
	int iTestValue = 1000;	
	
	while ( 1 )  
	{
		for (unsigned int uIndex=0; uIndex<uIntDataSize; uIndex++)
			liData[uIndex] = iTestValue;
			
		printf("Bld send to %x port %d Value %d\n", uAddr, uPort, iTestValue);		
		int iFailSend = BldClientSendRawData(pVoidBldClient, uIntDataSize*sizeof(int), 
		  (char*) liData);
		  
		if ( iFailSend != 0 )
		{
			printf( "*** Send Fail\n" );
			break;
		}
		
		iTestValue++;		
		
		printf ("Press Enter to continue, or press q + Enter to Exit...\n" );
		int c = getchar(); // Waiting for keyboard interrupt to break the infinite loop

		if ( c == 'q' || c == 'Q' ) break;		
	}	
	
	free(liData);	
	BldClientRelease(pVoidBldClient);
	
	return 0;
}

} // extern "C" 
