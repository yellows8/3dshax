Result AM_Initialize(Handle handle, Handle* out);
Result AM_StartInstallCIADB0(Handle handle, Handle* out, u8 mediatype);
Result AM_StartInstallCIADB1(Handle handle, Handle* out);
Result AM_AbortCIAInstall(Handle handle, Handle ciaFileHandle);
Result AM_CloseFileCIA(Handle handle, Handle ciaFileHandle);
Result AM_FinalizeTitlesInstall(Handle handle, u8 mediatype, u32 total_titles, u64 *titleidlist, u8 unku8);
Result AM_DeleteTitle(Handle handle, u8 mediatype, u64 titleid);
Result AM_DeleteApplicationTitle(Handle handle, u8 mediatype, u64 titleid);

