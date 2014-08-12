#ifndef YLS8AES_H
#define YLS8AES_H

void aes_mutexenter();
void aes_mutexleave();
void aes_set_ctr(u32 *ctr);
void aes_set_iv(u32 *iv);
void aes_select_key(u32 keyslot);
void aes_select_key(u32 keyslot);
void aes_set_ykey(u32 keyslot, u32 *key);
void aes_set_xkey(u32 keyslot, u32 *key);
void aes_set_key(u32 keyslot, u32 *key);
void aes_ctr_crypt(u32 *buf, u32 size);
void aes_cbc_decrypt(u32 *buf, u32 size);
void aes_cbc_encrypt(u32 *buf, u32 size);

#endif
