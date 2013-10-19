#include <winsock2.h>
#include <iphlpapi.h>
#pragma  comment (lib, "ws2_32.lib")
// Link with Iphlpapi.lib
#pragma comment(lib, "IPHLPAPI.lib")
#include <string.h>
#include <errno.h>

#include "log.h"
#include "intradllcomm.h"

/**
 *   Sender's ctor
 */
IntraComm :: IntraComm ()
{
	for (int i=0; i < NSOCK; ++i) {
		iqSocket[i] = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
		if(iqSocket [i] < 0) {
			LOG(("create socket failed for iqSocket #%d: %s\n", i, strerror(errno) ));
		}

		struct sockaddr_in daddr;
		memset(&daddr, 0, sizeof(daddr));
		daddr.sin_family		= AF_INET;
		daddr.sin_addr.s_addr	= inet_addr ("127.0.0.1");
		daddr.sin_port			= htons (portBase+i);

		if ( connect (iqSocket[i], (struct sockaddr*) &daddr, sizeof(daddr)) < 0) {
			LOG(("connect socket failed for iqSocket #%d: %s\n", i, strerror(errno) ));
		}
	}
}

/**
 *   Receivers's ctor
 */
IntraComm :: IntraComm (unsigned ch): channel(ch)
{
	// create the receiver socket on the proper channel

}

IntraComm :: ~IntraComm ()
{

}


int IntraComm :: send (unsigned channel, unsigned char *buf, int len)
{
	int rc = ::send (iqSocket[channel], (const char *)buf, len, 0);
	if (rc != len) { LOG(("ERROR writing socket for channel: %d: %s\n", channel, strerror(errno))); }
	return rc;
}

/**
 *   receive method has to be redefined in a derived class in order to do the job
 */
int IntraComm :: receive (unsigned channel, unsigned char *buf, int len)
{
	return 0;
}

int IntraComm :: startReceive ()
{
	// create receive socket
	ss = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (ss < 0) {
        LOG(("create socket failed for server IQ socket: %s\n", strerror (errno) ));
        return -1;
    }
    int reuse = 1;
    setsockopt(ss, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));

	struct sockaddr_in addr;
    
	memset (&addr, 0, sizeof(addr));
    addr.sin_family       = AF_INET;
    addr.sin_addr.s_addr  = inet_addr ("127.0.0.1");
    addr.sin_port         = htons (portBase+channel);

    if (bind (ss, (struct sockaddr*)&addr, sizeof(addr)) < 0 ) {
        LOGX("bind socket failed for server IQ socket: %s\n", strerror(errno));
        return -1;
    } else {
		LOGX("BOUND on port: %d\n", (portBase+channel) );
	}

	// start a receive thread to get discovery responses
	int rc = pthread_create ( &receiveTid, NULL, IntraComm :: rxThread, this);
	if (rc != 0) {
		LOGX("pthread_create failed: rc=%d\n", rc);
		return -1;
	} else {
		LOGX("%s\n", "thread succcessfully started");
		return 0;
	}
	
}

// private:

void * IntraComm :: rxThread (void *arg)
{
	IntraComm *t = (IntraComm *)arg;
	unsigned char buffer [20480];
	int c = 0;
	long int brx = 0;

	while (1) {
		int nr = recvfrom ( t->ss, (char*)&buffer, sizeof(buffer), 0, 0, 0);
		if ( nr < 0 ) {
			LOGX("recvfrom socket failed: %s\n", strerror(errno));
            break;
        }
		if ( nr == 0 ) {
			LOGX("recvfrom socket: %s received\n", "empty buffer");
			break;
        }

		if ( nr > 0 ) {
			t->receive (t->channel, buffer, nr);
			c++;
			brx += nr;
		}
		if ((c % 1024) == 0) {
			LOG((">>>>>>>>>>>>>>>>> stat: %d %ld %d\n", nr, brx, (brx/nr)));
		}
	}

	return 0;
}

//	unsigned channel;
//	int ss [8];

 