//SERVERMSG for communicate to translate
//MESSAGE for translate to communicate
#include <netinet/in.h>
#include "clientmsg.h"

#ifndef _servermsg
#define _servermsg

#ifndef CMSGLEN
#define CMSGLEN 100
#endif


struct SERVERMSG{
	int OP;
	char username[20];
	char buf[CMSGLEN];
	struct sockaddr_in client;
	int qid;
};

struct MESSAGE{
	long msgtype;
	struct SERVERMSG msg;
};

#endif
