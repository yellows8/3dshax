Codebase for Nintendo 3DS modded-FIRM("CFW"), for both Old3DS/New3DS. This also includes ctrserver, which is a network server which runs on the 3DS. This codebase originally started at roughly December 2012 - January 2013, which was finally released in January 2017.

You *must* build this yourself. This codebase is *only* intended for those that can build this codebase themselves. Built binaries will+should not be released(hence the below build options). Hence, this is mainly for developers.

Kind of like other "CFW", use at your own risk if you do anything (semi-)dangerous on physnand without nandredir & without a way to restore a nandimage.

# Building
To build: make {optional makefile parameters listed below}  
You have to use various "DISABLE" build options if you don't use the "LOADA9_NEW3DSMEM=1" build option, otherwise 3dshax will fail to build.
You will likely want to have your own script(s) for the default build command(s) you use.

Building ctrserver requires "3dshaxarm11_ctrserver_data/auth.bin", this is the "auth.txt" which would be used on the client-side. This is a <=0x100-byte file, the contents can be anything(random data for example).

The end result of building both of these is "3dshax_arm9.bin" and "3dshax_arm11_ctrserver.bin". After building, each .bin is copied to OUTPATH, for the latter .bin it's copied to OUTPATH/3dshax_arm11.bin.

# Usage

By default NAND-redir is enabled, see below for disabling it. When NAND-redir is enabled, the base sector-num must be specified via Makefile parameter NANDREDIR_SECTORNUM.

By default the FIRM-launch code loads the plaintext FIRM from SD "/firm.bin", see the below *_FIRMLAUNCH_LOAD* options. The FWVER values used by the arm9code is automatically determined via the arm11kernel configmem init code in the FIRM being loaded.

The arm9 codebase will handle launching the sdcard "/3dshax_arm11.bin", when enabled(which is the default). By default this arm11code is loaded into the "dlp" process, this can be overridden with "spider" by holding down the "Up" D-Pad button when main() is executed, or the "Down" D-Pad button there instead for running under the Miiverse applet. The server must not be running under dlp when trying to boot into \*hax payload with Old3DS.

If you press buttons X, Select, and Start, while the arm9-thread is running, the thread will dump FCRAM+AXIWRAM to SD then terminate the thread. You should shutdown the system soon after using this.

When DISABLE_ARM11KERNEL_DEBUG isn't(?) used, 3dshax may randomly fail to boot. When this happens just reboot and try again. This is probably related to the arm9<>arm11 IPC sync done by the arm9-thread/etc?

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
* "DISABLE_ARM11KERNEL_SVCHANDLER_PATCH=1" Disables the ARM11-kernel svc-handler patch, which when the patch is left enabled allows access to all SVCs. This option also disables patching the svc7b entry in the svc jumptable when that entry is 0x0, like with FIRM >=v11.0.
* "DISABLE_NETDEBUG=1" Disable the loop in the arm11kernel debug exception handler(when procstarthook/arm11kernel_debug are enabled) which waits for a signal(terminate process / continue) from ctrserver.
* "ENABLE_CMDLOGGING=1" Enables ARM11-kernel patches+code for logging commands.
* "CMDLOGGING_PADCHECK=value" For cmd-logging, only do logging when any of the bits in the specified value is set for the current PAD register state.
* "CMDLOGGING_PROCNAME0=value" u32 value to compare the src/dst process-name with for filtering. This is required for cmd-logging.
* "CMDLOGGING_PROCNAME1=value" Optional extra u32 value to compare the src/dst process-name with for filtering, for cmd-logging. When this is set, only commands where the src and dst are CMDLOGGING_PROCNAME0 and CMDLOGGING_PROCNAME1 are logged(for example, for logging commands sent to a certain sysmodule sent by a certain process).
* "CMDLOGGING_ALTPROCNAME=value" Optional extra u32 value to compare the src/dst process-name with for filtering, for cmd-logging. This can be used to log all service commands sent/received by the procnames specified  by CMDLOGGING_PROCNAME0 and CMDLOGGING_ALTPROCNAME(not just commands sent to each other like with CMDLOGGING_PROCNAME1). CMDLOGGING_PROCNAME1 must not be used when this option is used.
* "CMDLOGGING_IGNORE_PROCNAME=value" Optional u32 value to compare the src/dst process-name with, when it matches the cmd will not be logged.
* "CMDLOGGING_CMDHDR_FILTER=value" Optional command-request header value, when used only comand requests with this header will be logged.
* "ENABLE_ARM11KERNEL_SVCBREAKPATCH=1" This enables writing a bkpt instruction to the start of the ARM11-kernel code handling svcBreak.

* "DISABLE_GETEXHDRHOOK=1" Disables the arm9 get-exheader hook. This option must be used when DISABLE_ARM11KERNEL_DEBUG or DISABLE_ARM11KERNEL_PROCSTARTHOOK is set.
* "ENABLE_LOADA9_x01FFB800=1" This enables arm9 code which loads the 0x4800-byte SD file @ "/x01ffb800.bin" to arm9-mem 0x01ffb800. This should only be used when NAND-redir is enabled, and when the SD nandimage is originally from another 3ds, converted for usage on another 3ds. This is broken on New3DS(openfile() in loadfile_charpath() never returns), LOADA9_x01FFB800_INCFILEPATH must be used here due to this.
* "LOADA9_x01FFB800_INCFILEPATH={filepath relative to the 3dshaxarm9 directory}" When ENABLE_LOADA9_x01FFB800 is specified, this does a 0x100-byte memcpy for writing to 0x01ffb800 with the specified file(which gets included in the 3dshax_arm9.bin), instead of loading from a file on SD.
* "ENABLE_ARM11PROCLIST_OVERRIDE=1" This enables overriding the exheader and/or the loaded code binary at process-start, for any process(see 3dshax_arm9.c).
* "ADDEXHDR_SYSMODULE_DEPENDENCY=hexvalue" This adds sysmodules to the specified process exheader, so that ctrserver under dlp module gets loaded + is accessible over the network(only needed for getting ctrserver to load eariler than normal).
* "ADDEXHDR_SYSMODULE_DEPENDENCY_PADCHECK=hexvalue" When used the code for ADDEXHDR_SYSMODULE_DEPENDENCY only gets executed when the specified PAD button(s) are pressed.
* "ENABLE_BROWSER_APPMEM=1" Patch the browser(spider/skater) exheader so that memregion=APPLICATION and reslimit=APPLICATION, this allows the process to allocate much more memory. The official browser code won't allocate additional memory with this. This was implemented for use with webkitdebug.s from here: https://github.com/yellows8/3ds_browserhax_common
* "ENABLE_GAMECARD=1" Enables 3ds gamecard reading code(including ctrcard cmd 0xc6 code).
* "ENABLE_ARM11CODELOAD_SERVACCESSCONTROL_OVERWRITE=1" Enable overwriting the exheader service-access-control for the arm11codeload with the get-exhdr-hook, with the list from 3dshax_arm9.c. This is only needed when the loaded arm11 binary is not the latest ctrserver binary which has access to all services.
* "DISABLE_FSACCESSINFO_OVERWRITE=1" Disable exheader FS accessinfo overwrite with all 0xFF for the arm11code-load process.
* "DISABLE_A9THREAD=1" Disables creation of the arm9 thread.
* "ENABLE_CONFIGMEM_DEVUNIT=1" Enables writing val0 to configmem UNITINFO. This can be used to enable dev-mode for ErrDisp. This should not be used normally(breaks eShop for example).

* "ENABLE_FIRMLAUNCH_HOOK=1" Enables hooking Process9 FIRM-launch once the system finishes fully booting after previous FIRM-launch(es). FIRM-launch parameters won't be cleared with this, so that launching titles with this works.
* "DISABLE_FIRMLAUNCH_LOADSD=1" Disable loading FIRM from SD from the firmlaunch hook. If this is used, then the ENABLE_FIRMLAUNCH_LOADNAND=1 option must be used.
* "ENABLE_FIRMLAUNCH_LOADNAND=1" Enable loading FIRM from NAND(from the default ExeFS filepath used by official firmlaunch) from the firmlaunch hook. This is only used when loading the FIRM from SD fails, the FIRM being launched isn't available on SD / isn't supported with SD loading, or when DISABLE_FIRMLAUNCH_LOADSD=1 is used. With the second NATIVE_FIRM firmlaunch, this will *only* load the FIRM from physnand, not the nandredir image. If this code gets executed in that case with nandredir enabled, it will abort(allowing the FIRM boot to continue would only result in boot failure due to nandimage incompatibility with the physnand FIRM, depending on the FIRM/nandimage versions). Therefore, with nandredir enabled this should only be used for non-nativefirm.
* "ALTSD_FIRMPATH=1" Use SD FIRM filepath "/3dshax_firm.bin" instead of "/firm.bin".
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

* "MEMDUMPBOOT_SRCADDR=address" Copies memory from the specified address to the dst with the configured size, the first time 3dshax runs with RUNNINGTYPE 3. This could be used to dump memory which would be overwritten / no longer available later in boot. Once 3dshax boots, you could then use 3dshaxclient to read the memory from the dst addr(see below).
* "MEMDUMPBOOT_DSTADDR=address" Override the default(0x08001000) dst address used with the above. The dst memory starts with magicnum "DUMP" followed by the actual data. The dump code only runs with RUNNINGTYPE 3 when the data at dst+0 doesn't match this magicnum.
* "MEMDUMPBOOT_SIZE=val" Override the default(0x200) size used with the above.

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
* v10.0
* v10.4
* v11.0 Something related to the svcBackdoor code might be broken or something?(The getdebuginfoblk network cmd works fine usually but sometimes returns nothing)
* v11.1
* v11.2
* v11.3 Broken
* {Versions that work fine as-is due to auto-location}

Basically all of the codebase automatically determines what addresses to patch/etc on-the-fly(besides some structure/etc stuff which use FWVER instead / minor stuff).

The codebase also determines the version of the FIRM being loaded on-the-fly, hence the only time this codebase needs updated for newer FIRM is when auto-location breaks / certain structures change.

Note: when using nandredir where you also want to use old NATIVE_FIRM, you should use ALTSD_FIRMPATH. The SD firm.bin should contain a FIRM compatible with physnand, while 3dshax_firm.bin should contain the actual FIRM you want to run under nandredir.

# Debugging

Debug data is always written to SD "/3dshax_debug.bin" unless "DISABLE_ARM11KERNEL_DEBUG=1" is used. The contents can be parsed with the 3dshax_parsedebug tool. This contains exception dumps, and cmdlog data if enabled via the build options. By default a process will not terminate at crash until a continue/kill command is used by 3dshaxclient, if the server is active(see the DISABLE_NETDEBUG option).

The cmdlogs are logs of IPC commands' request and reply data, with dumps of the data buffers(when --hexdump is passed to 3dshax_parsedebug). Note that in some cases some of the dumped data is invalid: it just gets the physaddr of buf+0 then writes the whole buffer size with that to SD. The result is invalid data when the physmem for a buffer isn't linear, in particular 0x04000000+ memory after the first page.

Cmdlogging will slow down the system, unless it's logging barely anything.

# Networking

This includes ctrserver and 3dshaxclient. The former is a RPC server which runs under dlp-sysmodule by default, see the Usage section. The protocol is based on [ctrclient](https://github.com/neimod/ctr/tree/master/ramtracer/ctrclient), some code is used from that as well. 3dshaxclient is built with that(unless you build with ctr-cryptotool for the CTRCLIENT build option from 3dscrypto-tools). Among other things, this allows doing debugging and AES-engine crypto over the network.

You can use the running server with [3dscrypto-tools](https://github.com/yellows8/3dscrypto-tools). DO NOT use network AES crypto without nandredir / a way to restore a nandimage: it's known to cause NAND corruption. In some rare(?) cases the decrypted data may be junk, just rerun the tool if that happens.

For accessing the server you can use 3dshaxclient. The commands for the client/server are not documented. See 3dshaxclient.c, network.c, and 3dshax_arm9.c.

The network debugging is done with the custom client commands for 3dshaxclient.

# Setup

Note: this entire Setup section of the README didn't exist in the README until 3dshax release.

## sighax / Custom FIRM

Remember, be *very* *careful* with this since NAND writing is done here. And of course, backup your NAND image first.

This could(not tested) also be used with non-BootROM FIRM-boot methods(from "CFW"/whatever), in that case you can skip/ignore the NAND-related info in this section.

Stage0, Building:

1. Build [bootldr9_rawdevice](https://github.com/yellows8/bootldr9_rawdevice). Use at least build options "ENABLE_RETURNFROMCRT0=1" and "ENABLE_CONTINUEWHEN_PAYLOADCALLFAILS=1". You can use whatever device/padcheck build options you want if any.
2. Build [firm_payload_bootstrap](https://github.com/yellows8/firm_payload_bootstrap) with the above built binary for bootldr9_rawdevice. Manually replace the signature in the built FIRM with your own signature.
3. Build [3dsbootldr_fatfs](https://github.com/yellows8/3dsbootldr_fatfs) with at least build option "ENABLE_RETURNFROMCRT0=1".
4. Build payloadbuilder from [bootldr9_rawdevice](https://github.com/yellows8/bootldr9_rawdevice), then run: <code>payloadbuilder 3dsbootldr_fatfs.customfirmfmt 3dsbootldr_fatfs.bin 0x080F4000</code>
5. Build [3dsbootldr_firm](https://github.com/yellows8/3dsbootldr_firm), with at least build option "ENABLE_RETURNFROMCRT0=1". Do not use the rawdevice/NAND build options.

Stage1, SD Setup:

1. On the SD card, write "3dsbootldr_fatfs.customfirmfmt" to the raw sectors starting at sector 8. Make sure the area that would be written here is empty / not used.
2. Setup the built 3dsbootldr_firm on SD the same way described in the below arm9loaderhax section.

Stage2, NAND Setup:

1. Rebuild 3dsbootldr_firm with at least the following build options: "ENABLE_RETURNFROMCRT0=1", "BINLOAD_DISABLE=1", "USE_RAWDEVICE=1", "USEDEVICE_NAND=1", "RAWDEVICE_STARTSECTOR={0x5A180}", and "RAWDEVICE_NUMSECTORS={0x800}".
2. Then run payloadbuilder from [bootldr9_rawdevice](https://github.com/yellows8/bootldr9_rawdevice): <code>payloadbuilder 3dsbootldr_firm.customfirmfmt 3dsbootldr_firm.bin 0x080F4000</code>
3. Write "3dsbootldr_firm.customfirmfmt" to the raw sectors in NAND starting at sector 8.
4. Write the official unmodified FIRM(ideally unmodified at least) that you want to boot from NAND, to NAND offset 0x0B430000(sector 0x5A180).

Stage3, custom FIRM Setup:

1. Write the FIRM built from firm_payload_bootstrap with the previously updated signature, to your system's NAND firm0 partition, using whatever method you want. If writing it raw, remember to encrypt it properly. If you want to use it with non-NAND FIRM boot instead, you'll have to manually encrypt the FIRM sections following the FIRM header + use a signature for non-NAND.

Note that "firm.bin" on SD is required with this, when booting 3dshax from SD with the above.

After building 3dshax with OUTPATH={sd root}, you must run the build_hashedbin.sh script from 3dsbootldr_firm with 3dshax_arm9.bin on SD, unless you built 3dsbootldr_firm with "DISABLE_BINVERIFY=1".

NOTE: Prior to broken attempts at using the 3dsbootldr repos mentioned in the arm9loaderhax section, from the beginning they(and others) were originally used with sighax.

NOTE: You MUST avoid triggering any FIRM-partition-installation on physnand(downloadplay, sysupdate, ...). Otherwise, official FIRM will overwrite the FIRM installed to NAND firm0(system will still boot fine but you would have to reinstall the custom FIRM).

## Public arm9loaderhax

Currently 3dshax can only be used with a9lh on Old3DS, not New3DS due to New3DS keys not being setup properly. If you run 3dsbootldr_fatfs as a Luma payload, your build of Luma *must* include [this](https://github.com/AuroraWright/Luma3DS/commit/f03e232b90ed3e2e8545c5abb4c489ca4f1e0b7d) commit.

3dshax was *never* used with any form of arm9loaderhax until some broken attempts in Jan-2017.

1. Setup [3dsbootldr_fatfs](https://github.com/yellows8/3dsbootldr_fatfs), where the output binary is used as your "arm9loaderhax.bin" or equivalent, on SD. "ALTARM11BOOT=1" must be used when building this. Do not use ENABLE_RETURNFROMCRT0.
2. Setup [3dsbootldr_firm](https://github.com/yellows8/3dsbootldr_firm). At offset 0x0 in 3dsbootldr_firm.bin, insert(not overwrite) raw bytes "00 80 0E 08" then write it to SD "/load9.bin". Likewise for 3dsbootldr_firm_arm11.bin, except use raw bytes "00 E7 FF 1F" then write to SD "/load11.bin". The binaries on SD must be run with the build_hashedbin.sh script from 3dsbootldr_fatfs, unless you used "DISABLE_BINVERIFY=1" for 3dsbootldr_fatfs. Do not use the rawdevice/NAND build options.

After building 3dshax with OUTPATH={sd root}, you must run the build_hashedbin.sh script from 3dsbootldr_firm with 3dshax_arm9.bin on SD, unless you built 3dsbootldr_firm with "DISABLE_BINVERIFY=1".

**NOTE: If you use this with DISABLENANDREDIR=1, you MUST avoid triggering any FIRM-partition-installation(downloadplay, sysupdate, ...). A brick(on New3DS at least) will occur otherwise. While there's some disabled code for some form of FIRM-protection, it was never really finished.**

## 3ds-totalcontrolhaxx

If you are on a compatible FIRM version, you can also use [3ds-totalcontrolhaxx](https://github.com/yellows8/3ds-totalcontrolhaxx). Just setup the .3dsx on SD(or even use the payload binary directly if you want) + the actual 3dshax, and that's all. The firm.bin on SD should match the physnand version. If you want to boot a different FIRM version, use 3dshax build option ALTSD_FIRMPATH.

