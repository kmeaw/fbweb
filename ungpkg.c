// Copyright 2010-2011 Sven Peter <svenpeter@gmail.com>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

// Thanks to Mathieulh for his C# retail unpacker
//  (http://twitter.com/#!/Mathieulh/status/23070344881381376)
// Thanks to Matt_P for his python debug unpacker
//  (https://github.com/HACKERCHANNEL/PS3Py/blob/master/pkg.py)

#include "pkgtypes.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

static u8 *pkg = NULL;
static u64 size;
static u64 offset;

#define fail(...)

// FIXME: use a non-broken sha1.c *sigh*
#include "aes.h"
#include "sha1.h"
static void sha1_fixup(struct SHA1Context *ctx, u8 *digest)
{
        u32 i;

        for(i = 0; i < 5; i++) {
                *digest++ = ctx->Message_Digest[i] >> 24 & 0xff;
                *digest++ = ctx->Message_Digest[i] >> 16 & 0xff;
                *digest++ = ctx->Message_Digest[i] >> 8 & 0xff;
                *digest++ = ctx->Message_Digest[i] & 0xff;
        }
}

void sha1(u8 *data, u32 len, u8 *digest)
{
        struct SHA1Context ctx;

        SHA1Reset(&ctx);
        SHA1Input(&ctx, data, len);
        SHA1Result(&ctx);

        sha1_fixup(&ctx, digest);
}

void sha1_hmac(u8 *key, u8 *data, u32 len, u8 *digest)
{
        struct SHA1Context ctx;
        u32 i;
        u8 ipad[0x40];
        u8 tmp[0x40 + 0x14]; // opad + hash(ipad + message)

        SHA1Reset(&ctx);

        for (i = 0; i < sizeof ipad; i++) {
                tmp[i] = key[i] ^ 0x5c; // opad
                ipad[i] = key[i] ^ 0x36;
        }

        SHA1Input(&ctx, ipad, sizeof ipad);
        SHA1Input(&ctx, data, len);
        SHA1Result(&ctx);

        sha1_fixup(&ctx, tmp + 0x40);

        sha1(tmp, sizeof tmp, digest);

}

void aes128ctr(u8 *key, u8 *iv, u8 *in, u64 len, u8 *out)
{
        AES_KEY k;
        u32 i;
        u8 ctr[16];
        u64 tmp;

        memset(ctr, 0, 16);
        memset(&k, 0, sizeof k);

        AES_set_encrypt_key(key, 128, &k);

        for (i = 0; i < len; i++) {
                if ((i & 0xf) == 0) {
                        AES_encrypt(iv, ctr, &k);

                        // increase nonce
                        tmp = be64(iv + 8) + 1;
                        wbe64(iv + 8, tmp);
                        if (tmp == 0)
                                wbe64(iv, be64(iv) + 1);
                }
                *out++ = *in++ ^ ctr[i & 0x0f];
        }
}


static void decrypt_retail_pkg(void)
{
	u8 key[0x10] = {
	  0x2E,
	  0x7B,
	  0x71,
	  0xD7,
	  0xC9,
	  0xC9,
	  0xA1,
	  0x4E,
	  0xA3,
	  0x22,
	  0x1F,
	  0x18,
	  0x88,
	  0x28,
	  0xB8,
	  0xF8,
	};
	u8 iv[0x10];

	if (be16(pkg + 0x06) != 1)
		fail("invalid pkg type: %x", be16(pkg + 0x06));

	memcpy(iv, pkg + 0x70, 0x10);
	aes128ctr(key, iv, pkg + offset, size, pkg + offset);
}

static void decrypt_debug_pkg(void)
{
	u8 key[0x40];
	u8 bfr[0x1c];
	u64 i;

	memset(key, 0, sizeof key);
	memcpy(key, pkg + 0x60, 8);
	memcpy(key + 0x08, pkg + 0x60, 8);
	memcpy(key + 0x10, pkg + 0x60 + 0x08, 8);
	memcpy(key + 0x18, pkg + 0x60 + 0x08, 8);

	sha1(key, sizeof key, bfr);

	for (i = 0; i < size; i++) {
		if (i != 0 && (i % 16) == 0) {
			wbe64(key + 0x38, be64(key + 0x38) + 1);	
			sha1(key, sizeof key, bfr);
		}
		pkg[offset + i] ^= bfr[i & 0xf];
	}
}

static void unpack_pkg(void)
{
	u64 i;
	u64 n_files;
	u32 fname_len;
	u32 fname_off;
	u64 file_offset;
	u32 flags;
	char fname[256];
	u8 *tmp;

	n_files = be32(pkg + 0x14);

	for (i = 0; i < n_files; i++) {
		tmp = pkg + offset + i*0x20;

		fname_off = be32(tmp) + offset;
		fname_len = be32(tmp + 0x04);
		file_offset = be64(tmp + 0x08) + offset;
		size = be64(tmp + 0x10);
		flags = be32(tmp + 0x18);

		if (fname_len >= sizeof fname)
			fail("filename too long: %s", pkg + fname_off);

		memset(fname, 0, sizeof fname);
		strncpy(fname, (char *)(pkg + fname_off), fname_len);

		flags &= 0xff;
		if (flags == 4)
			mkdir(fname, 0777);
		else if (flags == 1 || flags == 3)
		{
		  	FILE *f = fopen(fname, "wb");
			fwrite(pkg + file_offset, size, 1, f);
			fclose (f);
		}
		else
			fail("unknown flags: %08x", flags);
	}
}

void ungpkg(u8* _pkg)
{
	char *dir;
	pkg = _pkg;

	dir = malloc(10);
	memcpy(dir, pkg + 0x37, 9);
	dir[9] = 0;

	mkdir(dir, 0777);

	if (chdir(dir) != 0)
		fail("chdir(%s)", dir);

	offset = be64(pkg + 0x20);
	size = be64(pkg + 0x28);

	if (be16(pkg + 0x04) & 0x8000)
		decrypt_retail_pkg();
	else
		decrypt_debug_pkg();

	unpack_pkg();
}
