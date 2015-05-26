//Most of this code is by smea.

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <3ds.h>

#include "am.h"

Result AM_Initialize(Handle handle, Handle* out) //more like getLockHandle ?
{
        u32* cmdbuf=getThreadCommandBuffer();
        cmdbuf[0]=0x4120000; //request header code
       
        Result ret=0;
        if((ret=svcSendSyncRequest(handle)))return ret;
       
        if(out)*out=cmdbuf[3]; //used for a waitsync1...
       
        return cmdbuf[1];
}

//End of code by smea.

