#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/uio.h>
#include <net/if.h>
#include <string>

#include "multicastTestBasic.h"

using std::string;

/*
// Example main function
int main(int argc,char *argv[])
{
	char* sInterfaceIp = ( argc >= 2 ? argv[1] : NULL );
	
	return testMulticastSample( sInterfaceIp );
}
*/

extern "C"
int testMulticastSample(char* sInterfaceIp)
{
	const unsigned int uAddr = 239<<24 | 255<<16 | 0<<8 | 1; // multicast address	
	const unsigned int uPort = 50000;
	const unsigned char ucTTL = 32; /// minimum: 1 + (# of routers in the middle)
			  
	int iRetErrorCode = 0;
	
	try
	{
		/*
		 * socket and bind
		 */
		int iSocket	= socket(AF_INET, SOCK_DGRAM, 0);	
		if ( iSocket == -1 ) throw string("socket() == -1");
				
		sockaddr_in sockaddrSrc;
		sockaddrSrc.sin_family      = AF_INET;
		sockaddrSrc.sin_addr.s_addr = INADDR_ANY;
		sockaddrSrc.sin_port        = 0;

		if ( 
		  bind( iSocket, (struct sockaddr*) &sockaddrSrc, sizeof(sockaddrSrc) ) 
		  == -1 )
			throw string("bind() == -1");

		/*
		 * set multicast TTL and interface
		 */		
		if ( 
		  setsockopt( iSocket, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ucTTL, sizeof(ucTTL) ) 
		  < 0 ) 
			throw string("setsockopt(...IP_MULTICAST_TTL) failed");

		if ( sInterfaceIp != NULL) 
		{			
			in_addr address;
			address.s_addr = inet_addr(sInterfaceIp);
			
			if ( 
			  setsockopt( iSocket, IPPROTO_IP, IP_MULTICAST_IF, (char*)&address, 
			  sizeof(address) ) 
			  < 0) 
				throw string("setsockopt(...IP_MULTICAST_IF) failed");			
		}

		/*
		 * sendmsg
		 */		
		const char lcData[] = "MULTICAST BLD TEST"; // The sizeof(lcData) < uMaxDataSize
		const char* pData = lcData;
		const int iSizeData = sizeof(lcData);

		printf("Bld send to %x port %d Data String: %s\n", uAddr, uPort, lcData);
		
		sockaddr_in sockaddrDst;
		sockaddrDst.sin_family      = AF_INET;
		sockaddrDst.sin_addr.s_addr = htonl(uAddr);
		sockaddrDst.sin_port        = htons(uPort); 	
		
		struct iovec iov[2];
		iov[0].iov_base = (caddr_t)(pData);
		iov[0].iov_len  = iSizeData;
		
		struct msghdr hdr;
		hdr.msg_iovlen		= 1;		
		hdr.msg_name        = (sockaddr*) &sockaddrDst;
		hdr.msg_namelen		= sizeof(sockaddrDst);
		hdr.msg_control		= (caddr_t)0;
		hdr.msg_controllen	= 0;
		hdr.msg_iov			= &iov[0];

		unsigned int uSendFlags = 0;
		if ( 
		  sendmsg(iSocket, &hdr, uSendFlags) 
		  == -1 )
			throw string("sendmsg failed");
					
		/*
		 * close the socket
		 */
		close(iSocket);
	}
	catch (string& sError)
	{
		printf( "*** Error: %s, errno = %d (%s)\n", sError.c_str(), errno,
		  strerror(errno) );
		  
		iRetErrorCode = 1;
	}
		
	return iRetErrorCode;	
}
