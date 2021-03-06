PARAMS	:=	

ifneq ($(strip $(DISABLEAES)),)
	PARAMS	:=	$(PARAMS) DISABLEAES=1
endif

ifneq ($(strip $(DISABLENANDREDIR)),)
	PARAMS	:=	$(PARAMS) DISABLENANDREDIR=1
endif

ifneq ($(strip $(DISABLE_ARM9DEBUGGING)),)
	PARAMS	:=	$(PARAMS) DISABLE_ARM9DEBUGGING=1
endif

ifneq ($(strip $(DISABLE_ARM11KERNEL_DEBUG)),)
	PARAMS	:=	$(PARAMS) DISABLE_ARM11KERNEL_DEBUG=1
endif

ifneq ($(strip $(DISABLE_ARM11KERNEL_PROCSTARTHOOK)),)
	PARAMS	:=	$(PARAMS) DISABLE_ARM11KERNEL_PROCSTARTHOOK=1
endif

ifneq ($(strip $(DISABLE_ARM11KERNEL_SVCHANDLER_PATCH)),)
	PARAMS	:=	$(PARAMS) DISABLE_ARM11KERNEL_SVCHANDLER_PATCH=1
endif

ifneq ($(strip $(DISABLE_NETDEBUG)),)
	PARAMS	:=	$(PARAMS) DISABLE_NETDEBUG=1
endif

ifneq ($(strip $(ENABLE_ARM11KERNEL_SVCBREAKPATCH)),)
	PARAMS	:=	$(PARAMS) ENABLE_ARM11KERNEL_SVCBREAKPATCH=1
endif

ifneq ($(strip $(DISABLE_GETEXHDRHOOK)),)
	PARAMS	:=	$(PARAMS) DISABLE_GETEXHDRHOOK=1
endif

ifneq ($(strip $(ENABLE_LOADA9_x01FFB800)),)
	PARAMS	:=	$(PARAMS) ENABLE_LOADA9_x01FFB800=1
endif

ifneq ($(strip $(LOADA9_x01FFB800_INCFILEPATH)),)
	PARAMS	:=	$(PARAMS) LOADA9_x01FFB800_INCFILEPATH=$(LOADA9_x01FFB800_INCFILEPATH)
endif

ifneq ($(strip $(ENABLE_ARM11PROCLIST_OVERRIDE)),)
	PARAMS	:=	$(PARAMS) ENABLE_ARM11PROCLIST_OVERRIDE=1
endif

ifneq ($(strip $(ADDEXHDR_SYSMODULE_DEPENDENCY)),)
	PARAMS	:=	$(PARAMS) ADDEXHDR_SYSMODULE_DEPENDENCY=$(ADDEXHDR_SYSMODULE_DEPENDENCY)
endif

ifneq ($(strip $(ADDEXHDR_SYSMODULE_DEPENDENCY_PADCHECK)),)
	PARAMS	:=	$(PARAMS) ADDEXHDR_SYSMODULE_DEPENDENCY_PADCHECK=$(ADDEXHDR_SYSMODULE_DEPENDENCY_PADCHECK)
endif

ifneq ($(strip $(ENABLE_BROWSER_APPMEM)),)
	PARAMS	:=	$(PARAMS) ENABLE_BROWSER_APPMEM=1
endif

ifneq ($(strip $(ENABLE_GAMECARD)),)
	PARAMS	:=	$(PARAMS) ENABLE_GAMECARD=1
endif

ifneq ($(strip $(ENABLE_ARM11CODELOAD_SERVACCESSCONTROL_OVERWRITE)),)
	PARAMS	:=	$(PARAMS) ENABLE_ARM11CODELOAD_SERVACCESSCONTROL_OVERWRITE=1
endif

ifneq ($(strip $(DISABLE_FSACCESSINFO_OVERWRITE)),)
	PARAMS	:=	$(PARAMS) DISABLE_FSACCESSINFO_OVERWRITE=1
endif

ifneq ($(strip $(DISABLE_A9THREAD)),)
	PARAMS	:=	$(PARAMS) DISABLE_A9THREAD=1
endif

ifneq ($(strip $(ENABLE_CONFIGMEM_DEVUNIT)),)
	PARAMS	:=	$(PARAMS) ENABLE_CONFIGMEM_DEVUNIT=1
endif

ifneq ($(strip $(ENABLE_FIRMLAUNCH_HOOK)),)
	PARAMS	:=	$(PARAMS) ENABLE_FIRMLAUNCH_HOOK=1
endif

ifneq ($(strip $(DISABLE_FIRMLAUNCH_LOADSD)),)
	PARAMS	:=	$(PARAMS) DISABLE_FIRMLAUNCH_LOADSD=1
endif

ifneq ($(strip $(ENABLE_FIRMLAUNCH_LOADNAND)),)
	PARAMS	:=	$(PARAMS) ENABLE_FIRMLAUNCH_LOADNAND=1
endif

ifneq ($(strip $(ALTSD_FIRMPATH)),)
	PARAMS	:=	$(PARAMS) ALTSD_FIRMPATH=1
endif

ifneq ($(strip $(DISABLE_MATCHINGFIRM_HWCHECK)),)
	PARAMS	:=	$(PARAMS) DISABLE_MATCHINGFIRM_HWCHECK=1
endif

ifneq ($(strip $(ENABLE_BOOTSAFEFIRM_STARTUP)),)
	PARAMS	:=	$(PARAMS) ENABLE_BOOTSAFEFIRM_STARTUP=1
endif

ifneq ($(strip $(ENABLE_REGIONFREE)),)
	PARAMS	:=	$(PARAMS) ENABLE_REGIONFREE=$(ENABLE_REGIONFREE)
endif

ifneq ($(strip $(ENABLE_THEMECACHENAME)),)
	PARAMS	:=	$(PARAMS) ENABLE_THEMECACHENAME=$(ENABLE_THEMECACHENAME)
endif

ifneq ($(strip $(DISABLE_GAMECARDUPDATE)),)
	PARAMS	:=	$(PARAMS) DISABLE_GAMECARDUPDATE=1
endif

ifneq ($(strip $(ENABLE_OLDFS_AUTOLOCATE)),)
	PARAMS	:=	$(PARAMS) ENABLE_OLDFS_AUTOLOCATE=1
endif

ifneq ($(strip $(ENABLE_DMA)),)
	PARAMS	:=	$(PARAMS) ENABLE_DMA=$(ENABLE_DMA)
endif

ifneq ($(strip $(ENABLE_CMDLOGGING)),)
	PARAMS	:=	$(PARAMS) ENABLE_CMDLOGGING=1
endif

ifneq ($(strip $(CMDLOGGING_PADCHECK)),)
	PARAMS	:=	$(PARAMS) CMDLOGGING_PADCHECK=$(CMDLOGGING_PADCHECK)
endif

ifneq ($(strip $(CMDLOGGING_PROCNAME0)),)
	PARAMS	:=	$(PARAMS) CMDLOGGING_PROCNAME0=$(CMDLOGGING_PROCNAME0)
endif

ifneq ($(strip $(CMDLOGGING_PROCNAME1)),)
	PARAMS	:=	$(PARAMS) CMDLOGGING_PROCNAME1=$(CMDLOGGING_PROCNAME1)
endif

ifneq ($(strip $(CMDLOGGING_ALTPROCNAME)),)
	PARAMS	:=	$(PARAMS) CMDLOGGING_ALTPROCNAME=$(CMDLOGGING_ALTPROCNAME)
endif

ifneq ($(strip $(CMDLOGGING_IGNORE_PROCNAME)),)
	PARAMS	:=	$(PARAMS) CMDLOGGING_IGNORE_PROCNAME=$(CMDLOGGING_IGNORE_PROCNAME)
endif

ifneq ($(strip $(CMDLOGGING_CMDHDR_FILTER)),)
	PARAMS	:=	$(PARAMS) CMDLOGGING_CMDHDR_FILTER=$(CMDLOGGING_CMDHDR_FILTER)
endif

ifneq ($(strip $(ENABLE_DUMP_NANDIMAGE)),)
	PARAMS	:=	$(PARAMS) ENABLE_DUMP_NANDIMAGE=1
endif

ifneq ($(strip $(NANDREDIR_SECTORNUM)),)
	PARAMS	:=	$(PARAMS) NANDREDIR_SECTORNUM=$(NANDREDIR_SECTORNUM)
endif

ifneq ($(strip $(NANDREDIR_SECTORNUM_PADCHECK0)),)
	PARAMS	:=	$(PARAMS) NANDREDIR_SECTORNUM_PADCHECK0=$(NANDREDIR_SECTORNUM_PADCHECK0)
endif

ifneq ($(strip $(NANDREDIR_SECTORNUM_PADCHECK0VAL)),)
	PARAMS	:=	$(PARAMS) NANDREDIR_SECTORNUM_PADCHECK0VAL=$(NANDREDIR_SECTORNUM_PADCHECK0VAL)
endif

ifneq ($(strip $(NANDREDIR_SECTORNUM_PADCHECK1)),)
	PARAMS	:=	$(PARAMS) NANDREDIR_SECTORNUM_PADCHECK1=$(NANDREDIR_SECTORNUM_PADCHECK1)
endif

ifneq ($(strip $(NANDREDIR_SECTORNUM_PADCHECK1VAL)),)
	PARAMS	:=	$(PARAMS) NANDREDIR_SECTORNUM_PADCHECK1VAL=$(NANDREDIR_SECTORNUM_PADCHECK1VAL)
endif

ifneq ($(strip $(LOADA9_NEW3DSMEM)),)
	PARAMS	:=	$(PARAMS) LOADA9_NEW3DSMEM=1
endif

ifneq ($(strip $(LOADA9_FCRAM)),)
	PARAMS	:=	$(PARAMS) LOADA9_FCRAM=1
endif

ifneq ($(strip $(NEW3DS_MEMDUMPA9_DISABLEVRAMCLR)),)
	PARAMS	:=	$(PARAMS) NEW3DS_MEMDUMPA9_DISABLEVRAMCLR=1
endif

ifneq ($(strip $(NEW3DS_MEMDUMPA9_ADR)),)
	PARAMS	:=	$(PARAMS) NEW3DS_MEMDUMPA9_ADR=$(NEW3DS_MEMDUMPA9_ADR)
endif

ifneq ($(strip $(NEW3DS_MEMDUMPA9_SIZE)),)
	PARAMS	:=	$(PARAMS) NEW3DS_MEMDUMPA9_SIZE=$(NEW3DS_MEMDUMPA9_SIZE)
endif

ifneq ($(strip $(NEW3DS_ARM9BINLDR_PATCHADDR0)),)
	PARAMS	:=	$(PARAMS) NEW3DS_ARM9BINLDR_PATCHADDR0=$(NEW3DS_ARM9BINLDR_PATCHADDR0)
endif

ifneq ($(strip $(NEW3DS_ARM9BINLDR_PATCHADDR0_VAL)),)
	PARAMS	:=	$(PARAMS) NEW3DS_ARM9BINLDR_PATCHADDR0_VAL=$(NEW3DS_ARM9BINLDR_PATCHADDR0_VAL)
endif

ifneq ($(strip $(NEW3DS_ARM9BINLDR_PATCHADDR1)),)
	PARAMS	:=	$(PARAMS) NEW3DS_ARM9BINLDR_PATCHADDR1=$(NEW3DS_ARM9BINLDR_PATCHADDR1)
endif

ifneq ($(strip $(NEW3DS_ARM9BINLDR_PATCHADDR1_VAL)),)
	PARAMS	:=	$(PARAMS) NEW3DS_ARM9BINLDR_PATCHADDR1_VAL=$(NEW3DS_ARM9BINLDR_PATCHADDR1_VAL)
endif

ifneq ($(strip $(NEW3DS_ARM9BINLDR_CLRMEM)),)
	PARAMS	:=	$(PARAMS) NEW3DS_ARM9BINLDR_CLRMEM=$(NEW3DS_ARM9BINLDR_CLRMEM)
endif

ifneq ($(strip $(NEW3DS_ARM9BINLDR_CLRMEM_SIZE)),)
	PARAMS	:=	$(PARAMS) NEW3DS_ARM9BINLDR_CLRMEM_SIZE=$(NEW3DS_ARM9BINLDR_CLRMEM_SIZE)
endif

ifneq ($(strip $(ENABLE_NIMURLS_PATCHES)),)
	PARAMS	:=	$(PARAMS) ENABLE_NIMURLS_PATCHES=1
endif

ifneq ($(strip $(NIMPATCHURL_UPDATE)),)
	PARAMS	:=	$(PARAMS) NIMPATCHURL_UPDATE=$(NIMPATCHURL_UPDATE)
endif

ifneq ($(strip $(NIMPATCHURL_ECOMMERCE)),)
	PARAMS	:=	$(PARAMS) NIMPATCHURL_ECOMMERCE=$(NIMPATCHURL_ECOMMERCE)
endif

ifneq ($(strip $(ENABLE_LOADSD_AESKEYS)),)
	PARAMS	:=	$(PARAMS) ENABLE_LOADSD_AESKEYS=1
endif

ifneq ($(strip $(MEMDUMPBOOT_SRCADDR)),)
	PARAMS	:=	$(PARAMS) MEMDUMPBOOT_SRCADDR=$(MEMDUMPBOOT_SRCADDR)
	
	ifneq ($(strip $(MEMDUMPBOOT_DSTADDR)),)
		PARAMS	:=	$(PARAMS) MEMDUMPBOOT_DSTADDR=$(MEMDUMPBOOT_DSTADDR)
	else
		PARAMS	:=	$(PARAMS) MEMDUMPBOOT_DSTADDR=0x08001000
	endif
	
	ifneq ($(strip $(MEMDUMPBOOT_SIZE)),)
		PARAMS	:=	$(PARAMS) MEMDUMPBOOT_SIZE=$(MEMDUMPBOOT_SIZE)
	else
		PARAMS	:=	$(PARAMS) MEMDUMPBOOT_SIZE=0x200
	endif
endif

.PHONY: clean

all:	3dshax_arm9.bin 3dshax_arm11_ctrserver.bin

clean:
	make clean -f Makefile.arm9
	make clean -f Makefile_ctrserver.arm11 CTRULIB=$(CTRULIB)

export OUTPATH
export ARM9BINCPOUT_PATH

3dshax_arm9.bin:
	make -f Makefile.arm9 $(PARAMS)

3dshax_arm11_ctrserver.bin:
	make -f Makefile_ctrserver.arm11
