//u32 pxibuf[0x200>>2];

/*void dump_movablesed()
{
	void (*readmovablesed)(u32*, u32*) = (void*)0x8060c65;
	u32 movablesed[0x120>>2];

	memset(movablesed, 0, 0x120);
	readmovablesed(&pxifs_state[0x34a0>>2], movablesed);
	dumpmem(movablesed, 0x120);
}

u32 pxiam9_cmdgettitlecount(u32 *count, u8 mediatype)
{
	u32 *state;

	state = (u32*)*((u32*)0x8097980);
	state = &state[0x2888>>2];

	u32 (*funcptr)(u32*, u32*, u8) = (void*)0x8041c2d;
	return funcptr(state, count, mediatype);
}

u32 pxiam9_cmd3d(u32 *buf0, u32 buf0sz, u32 *buf1, u32 buf1sz, u32 *buf2, u32 buf2sz, u32 *buf3, u32 buf3sz, u32 flag)
{
	u32 *state;

	state = (u32*)*((u32*)0x8097980);
	state = &state[0x20>>2];

	u32 (*funcptr)(u32*, u32*, u32, u32*, u32, u32*, u32, u32*, u32, u32) = (void*)0x804562d;
	return funcptr(state, buf0, buf0sz, buf1, buf1sz, buf2, buf2sz, buf3, buf3sz, flag);
}

u32 pxiam9_cmd48(u8 *out, u8 mediatype)
{
	u32 *state;

	state = (u32*)*((u32*)0x8097980);

	u32 (*funcptr)(u32*, u8*, u8) = (void*)0x80481ed;
	return funcptr(state, out, mediatype);
}

u32 pxiam9_cmd4d(u64 titleid, u16 *path, u32 pathsize, u32 *outbuf, u32 outbufsize, u8 unk8)
{
	u32 *state;

	state = (u32*)*((u32*)0x8097980);
	state = &state[0x2a0c>>2];

	u32 (*funcptr)(u32*, u32, u64, u16*, u32, u32*, u32, u8) = (void*)0x8044599;
	return funcptr(state, 0, titleid, path, pathsize, outbuf, outbufsize, unk8);
}*/

/*u32 pxiam9_cmd2a(u8 mediatype, u32 *tidbuf, u32 totaltitles, u8 unk8)
{
	u32 *state;

	state = (u32*)*((u32*)0x8097980);

	u32 (*funcptr)(u32*, u8, u32*, u32, u8) = (void*)0x80477ad;
	return funcptr(state, mediatype, tidbuf, totaltitles, unk8);
}*/

/*void memctx_init(u32 *memctx, u32 *fcramadr, u32 size)
{
	u32 (*funcptr)(u32*, u32*, u32) = (void*)0x805a025;
	funcptr(memctx, fcramadr, size);
}

u32 dsiware_writefooter(u32 *hashbuf, u32 *memctx)
{
	u32 (*funcptr)(u32, u32, u32*, u32*, u8) = (void*)0x8043f7d;
	return funcptr(0, 0, hashbuf, memctx, 1);
}

void dsiware_test()
{
	u32 ret = 0;
	u32 pos, len;
	u32 *hashbuf = (u32*)(0x20703000-0x1000);
	u32 memctx[24>>2];

	char *path = (char*)"/footer_hashes.bin";//"sdmc:/dsiware.bin";
	u16 *filepath = (u16*)(0x20703000 - 0x100);

	memset(filepath, 0, 64*2);
	len = strlen(path);
	for(pos=0; pos<len; pos++)filepath[pos] = (u16)path[pos];

	if(loadfile(hashbuf, 0x1a0, filepath, (len+1) * 2)!=0)return;

	memset(((u32*)0x20703000), 0, 0x20004);

	*((u16*)0x804402e) = 0;
	// *((u16*)0x8044094) = 0;
	// *((u16*)0x804408e) = 0;
	svcFlushProcessDataCache((u32*)0x804402e, 2);
	//svcFlushProcessDataCache((u32*)0x8044094, 2);
	//svcFlushProcessDataCache((u32*)0x804408e, 2);

	memctx_init(memctx, (u32*)0x20703004, 0x20000);
	ret = dsiware_writefooter(hashbuf, memctx);

	//ret = pxiam9_cmd4d(0x000480044B513945, filepath, (len+1) * 2, ((u32*)0x20703004), 0x20000, 7);

	memcpy((u32*)0x20703004, (u32*)(getsp()-0x1000), 0x1000);

	*((u32*)0x20703000) = ret;
	dumpmem(((u32*)0x20703000), 0x20004);
}*/

/*void am9_stuff()
{
	u32 ret=0;
	u8 out;
	u64 *tidbuf = (u64*)0x20000000;//0x20703000;
	u32 *fcramptr = (u32*)tidbuf;*/

	//memset(((u32*)0x20703000), 0, 8);
	//tidbuf[0] = 0x0004001000021900LL;

	//memset(fcramptr, 0xffffffff, 0x2800 + 0x1c+8);

	/*loadfile(fcramptr, 0x2800 + 0x3c+8, input_filepath, 0x24);

	memset(framebuf_addr, 0x33333333, 0x46500);
	memset(&framebuf_addr[(0x46500)>>2], 0x33333333, 0x46500);*/

	//((u32*)0x809039c)[0] = arm9_stub[0];
	//((u32*)0x809039c)[1] = arm9_stub[1];
	//((u32*)0x808f4b4)[2] = arm9_stub[2];
	//svcFlushProcessDataCache((u32*)0x809039c, 8);

	//*tidbuf = 0x000400300000CE02LL;

	//fcramptr[2] = pxiam9_cmd2a(0, fcramptr, 1, 0);

	//while(*((vu16*)0x10146000) & 2);

	/*fcramptr[0] = pxiam9_cmd3d(&fcramptr[0], 0xa00, &fcramptr[(0xa00)>>2], 0xa00, &fcramptr[(0xa00*2)>>2], 0xa00, &fcramptr[(0xa00*3)>>2], 0xa00+0x3c+8, 0);

	fcramptr[1] = pxiam9_cmd48(&out, 1);

	memcpy(&fcramptr[2], (u32*)(0x08028000+8), 0xD8000-8);
	dumpmem(fcramptr, 0xD8000+8);*/
//}

/*void debug_dbs()
{
	u32 infobuf[2];

	((u32*)0x806ccc0)[0] = arm9dbs_stub[0];
	((u32*)0x806ccc0)[1] = arm9dbs_stub[1];
	*((u32*)0x808f108) = 0xe3a00000;//Patch the pxiam9_cmd3d() call with: "mov r0, #0"
	svcFlushProcessDataCache((u32*)0x806ccc0, 8);
	svcFlushProcessDataCache((u32*)0x808f108, 4);
	*((u32*)0x20703000) = 0;

	//while(*((vu16*)0x10146000) & 2);

	memset(infobuf, 0, 8);
	infobuf[0] = pxiam9_cmd48((u8*)&infobuf[1], 1);//pxiam9_cmdgettitlecount(&infobuf[1], 1);
	dumpmem(infobuf, 8);
	//dumpmem((u32*)0x20703004, *((u32*)0x20703000));

	memset(framebuf_addr, 0xffffffff, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0xffffffff, 0x46500);
}*/

/*void rsactx_init(u32 *rsactx)
{
	void (*funcptr)(u32*) = (void*)0x80607e5;
	funcptr(rsactx);
}

void rsapubkctx_init(u32 *rsapubkctx, u32 *rsactx)
{
	void (*funcptr)(u32*, u32*) = (void*)0x805f899;
	funcptr(rsapubkctx, rsactx);
}

void rsapubkctx_destroy(u32 *rsapubkctx)
{
	void (*funcptr)(u32*) = (void*)0x804e599;
	funcptr(rsapubkctx);
}

u32 rsapubkctx_cryptmsg(u32 *rsapubkctx, u32 *outmsg, u32 *insig, u32 sigsize)
{
	u32 *vtable = (u32*)rsapubkctx[0];

	u32 (*funcptr)(u32*, u32*, u32*, u32) = (void*)vtable[0];
	return funcptr(rsapubkctx, outmsg, insig, sigsize);
}

void rsa_test()
{
	u32 pos;
	u32 rsasize = 0x100;
	u32 *fcramptr = (u32*)0x20000000;
	u32 rsactx[0x208>>2];
	u32 rsapubkctx[0x44>>2];

	rsactx_init(rsactx);

	memset(rsactx, 0xffffffff, 0x100);//modulo
	rsactx[0x100>>2] = 1;//exponent
	rsactx[0x200>>2] = rsasize<<3;//RSA bitsize
	rsactx[0x204>>2] = 0;//0 = public exponent

	rsapubkctx_init(rsapubkctx, rsactx);
	rsapubkctx[0] = 0x080939e4;

	for(pos=0; pos<(rsasize>>2); pos++)fcramptr[pos+1] = pos | (pos<<8) | (pos<<16) | (pos<<24);

	fcramptr[0] = rsapubkctx_cryptmsg(rsapubkctx, &fcramptr[1 + (rsasize>>2)], &fcramptr[1], rsasize);

	rsapubkctx_destroy(rsapubkctx);

	dumpmem(fcramptr, 4 + rsasize*2);
}*/

/*void send_pxireply()
{
	u32 i;
	u32 pxi_id;
	u32 totalwords = 3;
	u32 pxidata[3];

	//pxidata[0] = 8;
	pxidata[1] = 0x00030040;
	pxidata[2] = 0xc8888888;

	pxi_id = 0;

	//for(pxi_id=0; pxi_id<16; pxi_id++)
	//{
		pxidata[0] = pxi_id;
		pxidata[2]++;
		for(i=0; i<totalwords; i++)
		{
			while(*((vu32*)0x10008004) & 2);

			*((vu32*)0x10008008) = pxidata[i];
		}
	//}
}*/

/*void write_loginfo(u8 *data, u32 datasize)
{
	u32 pos=0;
	u16 filepath[64];
	char *path = (char*)"/3dshax_debug.bin";

	if(fileobj_debuginfo==NULL)
	{
		memset(filepath, 0, 64*2);
		for(pos=0; pos<strlen(path); pos++)filepath[pos] = (u16)path[pos];

		if(openfile(sdarchive_obj, 4, filepath, (strlen(path)+1)*2, 7, &fileobj_debuginfo)!=0)return;
	}

	filewrite(fileobj_debuginfo, &datasize, 4, debuginfo_pos);
	debuginfo_pos+= 4;
	filewrite(fileobj_debuginfo, data, datasize, debuginfo_pos);
	debuginfo_pos+= datasize;
}*/

/*u32 mountcontent_openfile_hook_opensd(u32 *archiveclass, u32 **fileobj, u32 unk0, u32 *lowpath, u32 openflags, u32 unk1)
{
	u32 *lowpathdata = (u32*)lowpath[1];
	u32 pos;
	char *path = "/3dshax_title_code.bin";//"/3dshax_title_icon.bin";
	u16 filepath[256];

	//if(lowpath[2]!=0x14 || lowpathdata[3]!=0x6e6f6369)return 1;//Only handle ExeFS:/icon
	if(lowpath[2]!=0xc || lowpathdata[1]!=0x646f632e)return 1;//Only handle ExeFS:/.code

	write_loginfo(lowpathdata, lowpath[2]);

	memset(filepath, 0, 256*2);
	for(pos=0; pos<strlen(path); pos++)filepath[pos] = (u16)path[pos];

	if(openfile(sdarchive_obj, 4, filepath, (strlen(path)+1)*2, 1, fileobj)!=0)return 1;

	return 0;
}*/

/*void pxirecv()
{
	u32 pos=0;
	u32 i;
	u32 *fileobj = NULL;

	memset(framebuf_addr, 0x10101010, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0x10101010, 0x46500);

	memset(pxibuf, 0, 0x200);

	if(openfile(sdarchive_obj, 4, dump_filepath, 0x22, 7, &fileobj)!=0)return;

	while(*((vu16*)0x10146000) & 2)
	{
		if(*((vu32*)0x10008004) & 0x100)continue;

		pxibuf[pos] = *((vu32*)0x1000800c);
		pos++;
		if(pos >= 0x200>>2)break;
	}

	memset(pxibuf, 0xc0c0c0c0, 0x200);

	if(filewrite(fileobj, pxibuf, 0x200, 0)!=0)return;

	memset(framebuf_addr, pxibuf[0], 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], pxibuf[0], 0x46500);
}

void dump_pxirecv()
{
	memset(pxibuf, 0, 0x20);
	memset(framebuf_addr, 0x48484848, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0x48484848, 0x46500);
	launchcode_kernelmode(pxirecv);
	memset(framebuf_addr, 0x13333337, 0x46500);
	memset(&framebuf_addr[(0x46500+0x10)>>2], 0x13333337, 0x46500);
	dumpmem(pxibuf, 0x20);
}*/

/*void debug_memalloc()
{
	u32 *ptr = NULL;
	u32 *fileobj = NULL;

	u32* (*funcptr)(u32) = (void*)0x8060548;

	ptr = funcptr(0xffffffff);
	
	if(openfile(sdarchive_obj, 4, dump_filepath, 0x22, 7, &fileobj)!=0)return;
	if(filewrite(fileobj, &ptr, 4, 0)!=0)return;
	if(filewrite(fileobj, (u32*)0x08028004, 0xD8000-4, 4)!=0)return;
}*/

