#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <algorithm>

#include <dbDefs.h>
#include <registryFunction.h>
#include <subRecord.h>
#include <epicsExport.h>
#include <dbAddr.h>
#include <dbAccess.h>
#include <dbTest.h>

#include "multicastTestSub.h"
#include "multicastBLDLib.h"

/*
 * Local namespace. Not to be used outside this file.
 */
namespace
{

/*
 * Interface class for sending out Bld data
 *
 * design issue: No object semantics.
 */
class MulticastTestSub
{
public:	
	static int sendBld();
	
private:
	static const char* sPvBldAddr;
	static const char* sPvBldPort;
	static const char* sPvBldInterfaceIp;
	static const char* sPvBldPvList;
			
	/*
	 * Utility functions
	 */
	 
	/* multicast bld client */
	static int sendMulticastData( const char* sInterfaceIp, unsigned int uAddr, 
	  unsigned int uPort, int iDataSize, const char* pcData );
	 
	/* PV access and report */
	static int readPV(const char *sVariableName, int iBufferSize, void* pBuffer, short* piValueType );
	static int printPV(const char *sVariableName, int iBufferSize, void* pBuffer, short iValueType );
	static int sprintPV(const char *sVariableName, int iBufferSize, void* pBuffer, 
	  short iValueType, char* lcMsgBuffer );		
	MulticastTestSub(); // No object semantics.
};

}

extern "C"
{

int mySubDebug = 0;

typedef long (*processMethod)(subRecord *precord);

static long mySubInit(subRecord *precord,processMethod process)
{
    if (mySubDebug)
	{
        printf("mySubInit() : Record %s called mySubInit(%p, %p)\n",
               precord->name, (void*) precord, (void*) process);
	}

    return(0);
}

static long mySubProcess(subRecord *precord)
{
    if (mySubDebug)
	{
        printf("mySubProcess() : Record %s called mySubProcess(%p)\n",
               precord->name, (void*) precord);	
	}
	
	MulticastTestSub::sendBld();
		
    return(0);
}

/* Register these symbols for use by IOC code: */

epicsExportAddress(int, mySubDebug);
epicsRegisterFunction(mySubInit);
epicsRegisterFunction(mySubProcess);

} // extern "C"

/*
 * class MulticastTestSub
 */

/* static member functions */

int MulticastTestSub::sendBld()
{		
	short iFieldType;

	/* Read bld addr */
	long llBufAddr[16] = {0}; /* declare long buffer to ensure correct alignment */	
	if ( 	
	  readPV( sPvBldAddr, sizeof(llBufAddr), llBufAddr, &iFieldType )
	  != 0 )
	{
		printf( "mySubProcess() : readPV(%s) for PvBldAddr Failed\n", sPvBldAddr );
		return 1;
	}
	char* sBldAddr = (char*) llBufAddr;	
	unsigned int uAddr = ntohl( inet_addr( (char*) llBufAddr ) );

	/* Read Bld Port */
	long llBufPort[16] = {0}; 	
	if ( 
	  readPV( sPvBldPort, sizeof(llBufPort), llBufPort, &iFieldType )
	  != 0 )
	{
		printf( "mySubProcess() : readPV(%s) for PvBldPort Failed\n", sPvBldPort );
		return 2;
	}
	char* sBldPort = (char*) llBufPort;
	unsigned int uPort = atoi( (const char*) llBufPort );	

	/* Read Bld InterfaceIp */
	long llBufInterfaceIp[16] = {0}; 
	if ( 
	  readPV( sPvBldInterfaceIp, sizeof(llBufInterfaceIp), llBufInterfaceIp, &iFieldType )
	  != 0 )
	{
		printf( "mySubProcess() : readPV(%s) for PvBldInterface Failed\n", sPvBldInterfaceIp );
		return 3;
	}		
	char* sBldInterfaceIp = (char*) llBufInterfaceIp;
	
	/* Read Bld PvList */
	long llBufPvList[16] = {0}; 	
	if ( 
	  readPV( sPvBldPvList, sizeof(llBufPvList), llBufPvList, &iFieldType )
	  != 0 )
	{
		printf( "mySubProcess() : readPV(%s) for PvBldPvList Failed\n", sPvBldPvList );
		return 4;
	}
	char* sBldPvList = (char*) llBufPvList;
	
	/* Read Bld PvList
	   - Assume only ONE pv name is specified in the PvList
	 */
	long llBufPvVal[16] = {0}; 	
	if ( 
	  readPV( sBldPvList, sizeof(llBufPvVal), llBufPvVal, &iFieldType )
	  != 0 )
	{
		printf( "mySubProcess() : readPV(%s) for sBldPvList Failed\n", sBldPvList );
		return 5;
	}
	
	printf( "Sending Bld to Addr %s Port %s Interface %s PvList %s\n",
		sBldAddr, sBldPort, sBldInterfaceIp, sBldPvList );
	
	char lcMsgBuffer[64] = {0};
	if( 
	  sprintPV( sBldPvList, sizeof(llBufPvVal), llBufPvVal, iFieldType, lcMsgBuffer )
	  != 0 )
		return 6;
		
	sendMulticastData( sBldInterfaceIp, uAddr, uPort, strlen(lcMsgBuffer), lcMsgBuffer );
		
	return 0;
}

int MulticastTestSub::sendMulticastData( const char* sInterfaceIp, unsigned int uAddr, 
  unsigned int uPort, int iDataSize, const char* pcData )
{	
	const unsigned int uMaxDataSize =iDataSize; // in bytes
	const unsigned char ucTTL = 32; /// minimum: 1 + (# of routers in the middle)	
	
		
	EpicsBld::BldClientInterface* pBldClient = 
	  EpicsBld::BldClientFactory::createBldClient(uAddr, uPort, uMaxDataSize, 
	  ucTTL, sInterfaceIp);			
	  	  
	printf("Bld send to %x port %d Data String: %s\n", uAddr, uPort, pcData);
	int iFailSend = pBldClient->sendRawData(iDataSize, pcData);
		
	delete pBldClient;
	
	/*
	 * Error Report
	 */
	if ( iFailSend != 0 )
		printf( "[Error] multicastBldData() : pBldClient->sendRawData() failed\n" );
	
	return 0;
}

/*
 * static const member variables
 */
const char* MulticastTestSub::sPvBldAddr = "tomytsai:bldAddr";
const char* MulticastTestSub::sPvBldPort = "tomytsai:bldPort";
const char* MulticastTestSub::sPvBldInterfaceIp = "tomytsai:bldInterfaceIp";
const char* MulticastTestSub::sPvBldPvList = "tomytsai:bldPvList";

int MulticastTestSub::readPV(const char *sVariableName, int iBufferSize, void* pBuffer, short* piFieldType )
{	
    if (sVariableName == NULL || *sVariableName == 0 || pBuffer == NULL || piFieldType == NULL )
	{
		printf( "readPV(): Invalid parameter\n" );
    	return 1;
    }
	
	if ( (unsigned int) pBuffer & 0x3 != 0  )
	{
		printf( "readPV(): Buffer should be aligned in 32 bits boundaries\n" );
    	return 2;
	}
		
    DBADDR	dbaddrVariable;
    int iStatus = dbNameToAddr(sVariableName,&dbaddrVariable);	
    if ( iStatus != 0 )
	{
		printf("readPV(): dbNameToAddr(%s) failed. Status  = %d\n", sVariableName, iStatus);
		return(iStatus);
    }

    long lNumElements = std::min( (int) dbaddrVariable.no_elements,
	  (iBufferSize/dbaddrVariable.field_size) );
    long int lOptions=0;
	
    if(dbaddrVariable.dbr_field_type==DBR_ENUM) 
		iStatus = 
		  dbGetField(&dbaddrVariable,DBR_STRING,pBuffer,&lOptions,
		  &lNumElements,NULL);
    else
		iStatus = 
		  dbGetField(&dbaddrVariable,dbaddrVariable.dbr_field_type,pBuffer,&lOptions,
		  &lNumElements,NULL);
		  
    if ( iStatus != 0 )
	{
		printf("readPV(): dbGetField(%s) failed. Status  = %d\n", sVariableName, iStatus);
		return(iStatus);
    }
	
	*piFieldType = dbaddrVariable.dbr_field_type;

    return(0);
}

int MulticastTestSub::printPV(const char *sVariableName, int iBufferSize, void* pBuffer, 
	short iValueType )
{
	char lcMsgBuffer[64];
	int iSprintFail = 
	  sprintPV( sVariableName, iBufferSize, pBuffer, iValueType, lcMsgBuffer );
	  
	if ( iSprintFail != 0 ) return iSprintFail;
	 
	printf( "%s", lcMsgBuffer );
	return 0;
}

int MulticastTestSub::sprintPV(const char *sVariableName, int iBufferSize, void* pBuffer, 
	short iValueType, char* lcMsgBuffer )
{
    if (sVariableName == NULL || *sVariableName == 0 || pBuffer == NULL || lcMsgBuffer == NULL )
	{
		printf( "sprintPV(): Invalid parameter\n" );
    	return 1;
    }
	
	int iCharsPrinted = sprintf( lcMsgBuffer, "PV %s ", sVariableName );
	char* pcMsgBufferFrom = lcMsgBuffer + iCharsPrinted;
	
	switch (iValueType)
	{
	case DBR_STRING:
		sprintf( pcMsgBufferFrom, "(String) = %s", (char*) pBuffer );
		break;
	//case DBR_INT: // the same as DBR_SHORT, as defeind in <dbAccess.h>
	case DBR_SHORT:
		sprintf( pcMsgBufferFrom, "(short) = %d", *(short*) pBuffer );
		break;
	case DBR_FLOAT:
		sprintf( pcMsgBufferFrom, "(float) = %f", *(float*) pBuffer );
		break;
	case DBR_ENUM:
		sprintf( pcMsgBufferFrom, "(enum) = %d", *(int*) pBuffer );
		break;
	case DBR_CHAR:
		sprintf( pcMsgBufferFrom, "(char) = %c (%d)", *(char*) pBuffer, *(char*) pBuffer );
		break;
	case DBR_LONG:
		sprintf( pcMsgBufferFrom, "(long) = %ld", *(long*) pBuffer );
		break;
	default:
	case DBR_DOUBLE:
		sprintf( pcMsgBufferFrom, "(double) = %lf", *(double*) pBuffer );
		break;		
	}
	
	return 0;
}
