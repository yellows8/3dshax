Result srv_initialize(u32 use_srvpm);
void srv_shutdown();

Result srvpm_registerprocess(u32 procid, char *servicelist, u32 servicelist_size);
Result srvpm_unregisterprocess(u32 procid);
Result srvpm_replace_servaccesscontrol(char *servicelist, u32 servicelist_size);
Result srvpm_replace_servaccesscontrol_default();

