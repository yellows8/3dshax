Codebase for 3ds arm9 stuff + arm11 kernel/userland patches etc. This also includes ctrserver, which is a network server which runs on the 3ds.

# Building
To build just the non-ctrserver codebase: make -f Makefile.arm9 {other optional makefile parameters}  
To build just ctrserver: make -f Makefile_ctrserver.arm11 {other optional makefile parameters}  
To build arm9code+ctrserver: make {other optional makefile parameters}  

Building ctrserver requires "3dshaxarm11_ctrserver_data/auth.bin", this is the "auth.txt" which would be used on the client-side.

The end result of building both of these is "3dshax_arm9.bin" and "3dshax_arm11_ctrserver.bin". After building, each .bin is copied to OUTPATH, for the latter .bin it's copied to OUTPATH/3dshax_arm11.bin.

The arm9 codebase will handle launching the sdcard "/3dshax_arm11.bin", when enabled(which is the default). By default this arm11code is loaded into the "dlp" process, this can be overridden with "spider" by holding down the "Up" D-Pad button when main() is executed, or the "Down" D-Pad button there instead for running under the Miiverse applet.

Running the arm9 binary requires 3ds arm9haxx which can handle loading it. The first u32 is the load-address for the following data, for offset+4 see 3dshax9_start.s for the rest of the structure. With the default PRAM settings near the start of the .bin, the arm9code uses FW1F(system-version v4.1-v4.5) settings. The arm9 code loader must setup the PRAM parameter fields to the correct values, when not running on FW1F. The code-loader can ignore the PRAM structure when running on FW1F.

By default NAND-redir is enabled, see below for disabling it. When NAND-redir is enabled, the base sector-num must be specified via Makefile parameter NANDREDIR_SECTORNUM.

By default the FIRM-launch code loads the plaintext FIRM from SD "/firm.bin", see the below *_FIRMLAUNCH_LOAD* options. The FWVER values used by the arm9code is automatically determined via the arm11kernel configmem init code in the FIRM being loaded.

# Makefile parameters  
* "OUTPATH={path to sdcard root}" Optional, this can be used to copy the built binaries to the specified path.
* "ARM9BINCPOUT_PATH={path}" When OUTPATH is used, copy the built arm9bin to "$(OUTPATH)$(ARM9BINCPOUT_PATH)" instead of just "$(OUTPATH)".

* "DISABLEAES=1" Disables all arm9 AES code.
* "DISABLENANDREDIR=1" Disables NAND->SD redirection.
* "NANDREDIR_SECTORNUM={sectornum}" Sector-num base for the SD nandimage, for NAND-redir when enabled.
* "NANDREDIR_SECTORNUM_PADCHECK0={padbitmask}" When used with NANDREDIR_SECTORNUM_PADCHECK0VAL, the current NANDREDIR_SECTORNUM(during firmpatching @ firmlaunch) will be set to NANDREDIR_SECTORNUM_PADCHECK0VAL when all of the buttons specified by the NANDREDIR_SECTORNUM_PADCHECK0 bitmask are set.
* "NANDREDIR_SECTORNUM_PADCHECK0VAL={sectornum}" Required when using NANDREDIR_SECTORNUM_PADCHECK0, see above.
* "NANDREDIR_SECTORNUM_PADCHECK1={padbitmask}" "NANDREDIR_SECTORNUM_PADCHECK1VAL={sectornum}" Same as NANDREDIR_SECTORNUM_PADCHECK0* except for a different set of options, these are checked/used after NANDREDIR_SECTORNUM_PADCHECK0*.
* "ENABLE_DUMP_NANDIMAGE=1" Enables nandimage dumping to SD "/3dshax_dump.bin", when button Y is pressed once from the arm9 thread. To dump the physical NAND, "DISABLENANDREDIR=1" should be used.
* "LOADA9_NEW3DSMEM=1" Use the new3ds arm9 .ld for using new3ds arm9mem, for much more space.

* "DISABLE_ARM9DEBUGGING=1" Disables ARM9 exception debugging, this is automatically disabled when DISABLE_ARM11KERNEL_DEBUG=1 is used. Note that ARM9 exception dumping currently only works with ctrserver, this doesn't work for writing the exception info to SD.
* "DISABLE_ARM11KERNEL_DEBUG=1" Disables ARM11-kernel patches.
* "DISABLE_ARM11KERNEL_PROCSTARTHOOK=1" Disables the ARM11-kernel process-start hook, used for loading arm11code(ctrserver) etc. This isn't needed when "DISABLE_ARM11KERNEL_DEBUG=1" is used.
* "DISABLE_ARM11KERNEL_SVCHANDLER_PATCH=1" Disables the ARM11-kernel svc-handler patch, which when the patch is left enabled allows access to all SVCs.
* "DISABLE_NETDEBUG=1" Disable the loop in the arm11kernel debug exception handler(when procstarthook/arm11kernel_debug are enabled) which waits for a signal(terminate process / continue) from ctrserver.
* "ENABLE_CMDLOGGING=1" Enables ARM11-kernel patches+code for logging commands.
* "CMDLOGGING_PADCHECK=value" For cmd-logging, only do logging when any of the bits in the specified value is set for the current PAD register state.
* "CMDLOGGING_PROCNAME0=value" u32 value to compare the src/dst process-name with for filtering. This is required for cmd-logging.
* "CMDLOGGING_PROCNAME1=value" Optional extra u32 value to compare the src/dst process-name with for filtering, for cmd-logging. When this is set, only commands where the src and dst are CMDLOGGING_PROCNAME0 and CMDLOGGING_PROCNAME1 are logged(for example, for logging commands sent to a certain sysmodule sent by a certain process).
* "CMDLOGGING_ALTPROCNAME=value" Optional extra u32 value to compare the src/dst process-name with for filtering, for cmd-logging. This can be used to log all service commands sent/received by the procnames specified  by CMDLOGGING_PROCNAME0 and CMDLOGGING_ALTPROCNAME(not just commands sent to each other like with CMDLOGGING_PROCNAME1). CMDLOGGING_PROCNAME1 must not be used when this option is used.
* "CMDLOGGING_IGNORE_PROCNAME=value" Optional u32 value to compare the src/dst process-name with, when it matches the cmd will not be logged.
* "CMDLOGGING_CMDHDR_FILTER=value" Optional command-request header value, when used only comand requests with this header will be logged.
* "ENABLE_ARM11KERNEL_SVCBREAKPATCH=1" This enables writing a bkpt instruction to the start of the ARM11-kernel code handling svcBreak.

* "DISABLE_GETEXHDRHOOK=1" Disables the arm9 get-exheader hook. This option must be used when DISABLE_ARM11KERNEL_DEBUG or DISABLE_ARM11KERNEL_PROCSTARTHOOK is set, with ENABLE_ARM11CODELOAD_SERVACCESSCONTROL_OVERWRITE used, otherwise when doing a firm-launch the system will eventually trigger a fatal-error when dlp-module fails to get service-handles.
* "ENABLE_LOADA9_x01FFB800=1" This enables arm9 code which loads the 0x4800-byte SD file @ "/x01ffb800.bin" to arm9-mem 0x01ffb800. This should only be used when NAND-redir is enabled, and when the SD nandimage is originally from another 3ds, converted for usage on another 3ds. This is broken on New3DS(openfile() in loadfile_charpath() never returns), LOADA9_x01FFB800_INCFILEPATH must be used here due to this.
* "LOADA9_x01FFB800_INCFILEPATH={filepath relative to the 3dshaxarm9 directory}" When ENABLE_LOADA9_x01FFB800 is specified, this does a 0x100-byte memcpy for writing to 0x01ffb800 with the specified file(which gets included in the 3dshax_arm9.bin), instead of loading from a file on SD.
* "ENABLE_ARM11PROCLIST_OVERRIDE=1" This enables overriding the exheader and/or the loaded code binary at process-start, for any process(see 3dshax_arm9.c).
* "ADDEXHDR_SYSMODULE_DEPENDENCY=hexvalue" This adds sysmodules to the specified process exheader, so that ctrserver under dlp module gets loaded + is accessible over the network(only needed for getting ctrserver to load eariler than normal).
* "ADDEXHDR_SYSMODULE_DEPENDENCY_PADCHECK=hexvalue" When used the code for ADDEXHDR_SYSMODULE_DEPENDENCY only gets executed when the specified PAD button(s) are pressed.
* "ENABLE_SPIDER_APPMEM=1" Patch the spider exheader so that memregion=APPLICATION and reslimit=APPLICATION, this allows the spider process to allocate much more memory. The official spider code won't allocate additional memory with this. This was implemented for use with webkitdebug.s from here: https://github.com/yellows8/3ds_browserhax_common
* "ENABLE_GAMECARD=1" Enables 3ds gamecard reading code(including ctrcard cmd 0xc6 code).
* "ENABLE_ARM11CODELOAD_SERVACCESSCONTROL_OVERWRITE=1" Enable overwriting the exheader service-access-control for the arm11codeload with the get-exhdr-hook, with the list from 3dshax_arm9.c. This is only needed when the loaded arm11 binary is not the latest ctrserver binary which has access to all services.
* "DISABLE_FSACCESSINFO_OVERWRITE=1" Disable exheader FS accessinfo overwrite with all 0xFF for the arm11code-load process.
* "DISABLE_A9THREAD=1" Disables creation of the arm9 thread.
* "ENABLE_CONFIGMEM_DEVUNIT=1" Enables writing val0 to configmem UNITINFO. This can be used to enable dev-mode for ErrDisp.

* "ENABLE_FIRMLAUNCH_HOOK=1" Enables hooking Process9 FIRM-launch once the system finishes fully booting after previous FIRM-launch(es). FIRM-launch parameters won't be cleared with this, so that launching titles with this works.
* "DISABLE_FIRMLAUNCH_LOADSD=1" Disable loading FIRM from SD from the firmlaunch hook. If this is used, then the ENABLE_FIRMLAUNCH_LOADNAND=1 option must be used.
* "ENABLE_FIRMLAUNCH_LOADNAND=1" Enable loading FIRM from NAND(from the default ExeFS filepath used by official firmlaunch) from the firmlaunch hook. This is only used when loading the FIRM from SD fails, the FIRM being launched isn't available on SD / isn't supported with SD loading, or when DISABLE_FIRMLAUNCH_LOADSD=1 is used. With the second NATIVE_FIRM firmlaunch, this will *only* load the FIRM from physnand, not the nandredir image. If this code gets executed in that case with nandredir enabled, it will abort(allowing the FIRM boot to continue would only result in boot failure due to nandimage incompatibility with the physnand FIRM, depending on the FIRM/nandimage versions). Therefore, with nandredir enabled this should only be used for non-nativefirm.
* "DISABLE_MATCHINGFIRM_HWCHECK=1" Disables the code which checks that the FIRM is for the currently running system: no booting Old3DS FIRM on New3DS when that code is enabled, likewise for New3DS FIRM on Old3DS.
* "ENABLE_BOOTSAFEFIRM_STARTUP=1" Boot into SAFE_MODE_FIRM with the system-updater with nand-redir(if enabled), instead of nativefirm like normal. The FIRM code for Old3DS SAFE_MODE_FIRM is very old, hence the auto-locate code for nandredir+{proc9 hook for code executed from main()} doesn't work with safefirm atm.

* "ENABLE_REGIONFREE={val}" Enables the homemenu SMDH icon region check patch. This does not affect the region-lock via gamecard sysupdates, see DISABLE_GAMECARDUPDATE. "ENABLE_REGIONFREE=2" is the same as "ENABLE_REGIONFREE=1", except this also uses the "DISABLE_GAMECARDUPDATE=1" option at the same time. Note that this(SMDH region patch) may cause SD titles which normally aren't displayed, to be shown as presents or black icons.
* "DISABLE_GAMECARDUPDATE=1" Disables gamecard sysupdates, this is required for launching gamecards from other regions.

* "ENABLE_OLDFS_AUTOLOCATE=1" Enable auto-locating FS code with very old FIRM.

* "ENABLE_DMA=1" Enables the cmd for use via ctrserver for the DMA SVCs.

* "ENABLE_NIMURLS_PATCHES=1" Enables patching NIM so that it uses custom URLs for the ECommerceSOAP and NetUpdateSOAP URLs, see NIMPATCHURL_UPDATE and NIMPATCHURL_ECOMMERCE. Both of those options are required when using this ENABLE_NIMURLS_PATCHES option.
* NIMPATCHURL_UPDATE=URL Sets the NetUpdateSOAP URL to write to NIM with ENABLE_NIMURLS_PATCHES=1. The max length is 62 characters, not including the final null-terminator left in memory.
* NIMPATCHURL_ECOMMERCE=URL Sets the ECommerceSOAP URL to write to NIM with ENABLE_NIMURLS_PATCHES=1. See NIMPATCHURL_UPDATE for max len.

* "ENABLE_THEMECACHENAME=1" Enables patching home menu's theme cache filenames so that home menu uses its own theme cache under 3dshax. This is particularly useful when used with menuhax to a) avoid infinite boot loops and b) avoid ROP crashes in mismatching home menu versions.

* "ENABLE_LOADSD_AESKEYS=1" During firmlaunch, enable loading AES keyslots keydata from SD, see 3dshax_arm9.c.

* "NEW3DS_ARM9BINLDR_PATCHADDR0=address" Write NEW3DS_ARM9BINLDR_PATCHADDR0_VAL to {address}, from the new3ds arm9bin loader entrypoint hook.
* "NEW3DS_ARM9BINLDR_PATCHADDR0_VAL=val"
* "NEW3DS_ARM9BINLDR_PATCHADDR1=address"
* "NEW3DS_ARM9BINLDR_PATCHADDR1_VAL=val"
* "NEW3DS_ARM9BINLDR_CLRMEM=address" Clear the memory starting at address with size from NEW3DS_ARM9BINLDR_CLRMEM_SIZE, from the new3ds arm9bin loader entrypoint hook.
* "NEW3DS_ARM9BINLDR_CLRMEM_SIZE=val"

# FIRM Compatibility
Supported NATIVE_FIRM system-versions(versions where NATIVE_FIRM wasn't updated don't apply here):
* v4.1
* v5.x
* v6.x
* v7.x
* v8.x
* v9.0-v9.2
* v9.3
* v9.5
* v9.6
* v10.0 The LOADA9_NEW3DSMEM option is currently not usable starting with this version on new3ds, due to the baseaddr for the relocated Process9 NCCH(the tmp one used for loading) being moved from 0x080fffe0 to 0x0817ffe0(this mem-copy is done by kernel9). Using this again would require implementing a workaround.

The above doesn't really apply for functionality where addresses are located on-the-fly, unless specificly mentioned otherwise.

Most of the codebase automatically determines what addresses to patch on-the-fly. Some minor/commented-out patch(es) still uses hard-coded addresses for each FIRM version.

The codebase also determines the version of the FIRM being loaded on-the-fly, hence the only time this codebase needs updated for newer FIRM is when auto-patching breaks / arm11kernel structure(s) change. The former is basically rather rare anymore, unlike the latter.

## Code and patches which use hard-coded addresses
* ...

## Code and patches where the addresses are automatically determined on-the-fly
* NAND->SD redirection
* Process9 patch for hooking code called from Process9 main(), for getting code execution after FIRM-launch under Process9.
* Process9 FIRM-launch patches(for the function called by main() + addresses near the end of arm9mem).
* Process9 PxiFS code + state/vtable ptrs
* Process9 RSA sigcheck patches: certs(including tmd/tik), and the main RSA padding check func.
* Process9 AES mutex enter/leave functions
* CTRCARD cmd 0xc6 code/state ptrs
* ARM11-kernel patch addrs/etc
* ARM11-kernel cmd-logging patch addresses/etc
* ...
