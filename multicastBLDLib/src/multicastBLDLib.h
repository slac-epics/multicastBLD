#ifndef MULTICAST_BLD_LIB_H
#define MULTICAST_BLD_LIB_H

namespace EpicsBld
{	
/**
 * Abastract Interface of Bld Multicast Client 
 * 
 * The interface class for providing Bld Mutlicast Client functions.
 *
 * Design Issue:
 * 1. The value semantics are disabled. 
 */
class BldClientInterface
{
public:
	/**
	 * Send raw data out to the Bld Server
	 *
	 * @param iSizeData  size of data (in bytes)
	 * @param pData      pointer to the data buffer (char[] buffer)
	 * @return  0 if successful,  otherwise the "errno" code (see <errno.h>)
	 */
	virtual int sendRawData(int iSizeData, const char* pData) = 0;
	
	virtual ~BldClientInterface() {} /// polymorphism support
protected:	
	BldClientInterface() {} /// To be called from implementation class
private:
	///  Disable value semantics. No definitions (function bodies).
	BldClientInterface(const BldClientInterface&);
	BldClientInterface& operator=(const BldClientInterface&);
};

/**
 * Factory class of Bld Multicast Client 
 *
 * Design Issue:
 * 1. Object semantics are disabled. Only static utility functions are provided.
 * 2. Later if more factorie classes are needed, this class can have public 
 *    constructor, virtual destructor and virtual member functions.
 */
class BldClientFactory
{
public:
	/**
	 * Create a Bld Client object
	 *
	 * @param uAddr			mutlicast address. Usually with 239.0.0.1 ~ 239.255.255.255
	 * @param uPort			UDP port
	 * @param uMaxDataSize	Maximum Bld data size. Better to be less than MTU.
	 * @param ucTTL			TTL value in UDP packet. Ideal value is 1 + (# of middle routers)
	 * @param sInteraceIp	Specify the NIC by IP address (in c string format)
	 * @return				The created Bld Client object
	 */
	static BldClientInterface* createBldClient(unsigned uAddr, 
	  unsigned uPort, unsigned int uMaxDataSize, unsigned char ucTTL = 32, 
	  const char* sInteraceIp = 0);
		
	/**
	 * Create a Bld Client object
	 *
	 * Overloaded version
	 * @param uInteraceIp	Specify the NIC by IP address (in unsigned int format)
	 */
	static BldClientInterface* createBldClient(unsigned uAddr, 
	  unsigned uPort, unsigned int uMaxDataSize, unsigned char ucTTL = 32, 
	  unsigned int uInteraceIp = 0);
private:
	/// Disable object instantiation (No object semantics).
	BldClientFactory();
};

} // namespace EpicsBld

extern "C"
{
/* 
 * The following functions provide C wrappers for accesing EpicsBld::BldClientInterface
 * and EpicsBld::BldClientFactory
 */
 
/**
 * Init function: Use EpicsBld::BldClientFactory to construct the BldClient
 * and save the pointer in (*ppVoidBldClient)
 */
int BldClientInitByInterfaceName(unsigned uAddr, unsigned uPort, 
  unsigned int uMaxDataSize, unsigned char ucTTL, const char* sInterfaceIp, 
  void** ppVoidBldClient);
int BldClientInitByInterfaceAddress(unsigned uAddr, unsigned uPort, 
  unsigned int uMaxDataSize, unsigned char ucTTL, unsigned int uInterfaceIp, 
  void** ppVoidBldClient);	

/**
 * Release function: Call C++ delete operator to delete the BldClient
 */
int BldClientRelease(void* pVoidBldClient);	

/**
 * Call the Send function defined in EpicsBld::BldClientInterface 
 */
int BldClientSendRawData(void* pVoidBldClient, int iSizeData, char* pData);

} // extern "C"


#endif 
