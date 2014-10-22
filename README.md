Codebase for 3ds arm9 stuff + arm11 kernel/userland patches etc. This also includes ctrserver, which is a network server which runs on the 3ds.

# Building
To build just the non-ctrserver codebase: make -f Makefile.arm9 OUTPATH={path to sdcard root} {other optional makefile parameters}  
To build just ctrserver: make -f Makefile_ctrserver.arm11 OUTPATH={path to sdcard root} {other optional makefile parameters}  
To build arm9code+ctrserver:make -f Makefile_ctrserver OUTPATH={path to sdcard root} {other optional makefile parameters}  

Building ctrserver requires "3dshaxarm11_ctrserver_data/auth.bin", this is the "auth.txt" which would be used on the client-side.

The end result of building both of these is "3dshax_arm9.bin" and "3dshax_arm11_ctrserver.bin". After building, each .bin is copied to OUTPATH, for the latter .bin it's copied to OUTPATH/3dshax_arm11.bin.

The arm9 codebase will handle launching the sdcard "/3dshax_arm11.bin", when enabled(which is the default). By default this arm11code is loaded into the "dlp" process, this can be overridden with "spider" by holding down the "Up" D-Pad button when main() is executed.

Running the arm9 binary requires 3ds arm9haxx which can handle loading it. The first u32 is the load-address for the following data, for offset+4 see 3dshax9_start.s for the rest of the structure. With the default PRAM settings near the start of the .bin, the arm9code uses FW1F(system-version v4.1-v4.5) settings. The arm9 code loader must setup the PRAM parameter fields to the correct values, when not running on FW1F. The code-loader can ignore the PRAM structure when running on FW1F.

By default NAND-redir is enabled, see below for disabling it. When NAND-redir is enabled, the base sector-num must be specified via Makefile parameter NANDREDIR_SECTORNUM.

The FIRM-launch code loads the plaintext FIRM from SD "/firm.bin". The FWVER values used by the arm9code is automatically determined by checking the first u32 in the FIRM RSA signature.

# Makefile parameters  
* "DISABLEAES=1" Disables all arm9 AES code.
* "DISABLENANDREDIR=1" Disables NAND->SD redirection.
* "NANDREDIR_SECTORNUM={sectornum}" Sector-num base for the SD nandimage, for NAND-redir when enabled.
* "DISABLE_ARM11KERNEL_DEBUG=1" Disables ARM11-kernel patches.
* "DISABLE_ARM11KERNEL_PROCSTARTHOOK=1" Disables the ARM11-kernel process-start hook, used for loading arm11code(ctrserver) etc. This isn't needed when "DISABLE_ARM11KERNEL_DEBUG=1" is used.
* "DISABLE_ARM11KERNEL_SVCHANDLER_PATCH=1" Disables the ARM11-kernel svc-handler patch, which when the patch is left enabled allows access to all SVCs.
* "DISABLE_NETDEBUG=1" Disable the loop in the arm11kernel debug exception handler(when procstarthook/arm11kernel_debug are enabled) which waits for a signal(terminate process / continue) from ctrserver.
* "DISABLE_GETEXHDRHOOK=1" Disables the arm9 get-exheader hook. This option must be used when DISABLE_ARM11KERNEL_DEBUG/DISABLE_ARM11KERNEL_PROCSTARTHOOK is set, otherwise when doing a firm-launch the system will eventually trigger a fatal-error when dlp-module fails to get service-handles.
* "ENABLE_LOADA9_x01FFB800=1" This enables arm9 code which loads the SD file @ "/x01ffb800.bin" to arm9-mem 0x01ffb800. This should only be used when NAND-redir is enabled, and when the SD nandimage is originally from another 3ds, converted for usage on another 3ds.
* "ENABLE_GAMECARD=1" Enables 3ds gamecard reading code.
* "DISABLE_FSACCESSINFO_OVERWRITE=1" Disable exheader FS accessinfo overwrite with all 0xFF for the arm11code-load process.
* "DISABLE_A9THREAD=1" Disables creation of the arm9 thread.
* "ENABLE_CONFIGMEM_DEVUNIT=1" Enables writing val0 to configmem UNITINFO. This can be used to enable dev-mode for ErrDisp.
* "ENABLE_FIRMLAUNCH_HOOK=1" Enables hooking Process9 FIRM-launch once the system finishes fully booting after previous FIRM-launch(es). FIRM-launch parameters won't be cleared with this, so that launching titles with this works.
* "ENABLE_REGIONFREE={val}" Enables the homemenu SMDH icon region check patch. This does not affect the region-lock via gamecard sysupdates, see DISABLE_GAMECARDUPDATE. "ENABLE_REGIONFREE=2" is the same as "ENABLE_REGIONFREE=1", except this also uses the "DISABLE_GAMECARDUPDATE=1" option at the same time. Note that this(SMDH region patch) may cause SD titles which normally aren't displayed, to be shown as presents or black icons.
* "DISABLE_GAMECARDUPDATE=1" Disables gamecard sysupdates, this is required for launching gamecards from other regions.
* "ENABLE_CMDLOGGING=1" Enables ARM11-kernel patches+code for logging commands.
* "CMDLOGGING_PADCHECK=value" For cmd-logging, only do logging when any of the bits in the specified value is set for the current PAD register state.
* "CMDLOGGING_PROCNAME0=value" u32 value to compare the src/dst process-name with for filtering. This is required for cmd-logging.
* "CMDLOGGING_PROCNAME1=value" Optional extra u32 value to compare the src/dst process-name with for filtering, for cmd-logging.
* "ENABLE_DUMP_NANDIMAGE=1" Enables nandimage dumping to SD "/3dshax_dump.bin", when button Y is pressed once from the arm9 thread. To dump the physical NAND, "DISABLENANDREDIR=1" should be used.

# FIRM Compatibility
Supported NATIVE_FIRM system-versions(versions where NATIVE_FIRM wasn't updated don't apply here):
* v4.1
* v5.x (mostly)
* v6.x (mostly)
* v7.x
* v8.0

Some of the codebase automatically determines what addresses to patch on-the-fly. However some of the codebase still uses hard-coded addresses for each FIRM version.

## Code and patches which use hard-coded addresses
* Process9 RSA sigcheck patches: certs sigcheck + main RSA padding check func(Not supported on FIRM system-versions v5.x/v6.x).
* Process9 AES mutex enter/leave functions (Not supported on system-versions v5.x/v6.x)
* CTRCARD cmd 0xc6 code (Not supported on FIRM system-versions v5.x/v6.x)
* ARM11-kernel funcptrs and ARM11-kernel cmd-logging patch addresses (Cmd-logging is not supported on FIRM system-version v5.x/v6.x)
* ...

## Code and patches where the addresses are automatically determined on-the-fly
* NAND->SD redirection
* Process9 patch for hooking code called from Process9 main(), for getting code execution after FIRM-launch under Process9.
* Process9 FIRM-launch patches(for the function called by main() + addresses near the end of arm9mem).
* Process9 PxiFS code + state/vtable ptrs (This change to auto-locating these addrs resulted in file-writing breaking on >v4.1 FIRM)
* ...

