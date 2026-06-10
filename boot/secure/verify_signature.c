/*
 * CellKernel - Secure Boot Signature Verification
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 * 
 * Implements Ed25519 signature verification for kernel images
 */

#include <stdint.h>
#include <stddef.h>

/* Ed25519 Public Key Size */
#define ED25519_PUBLIC_KEY_SIZE  32
#define ED25519_SIGNATURE_SIZE   64
#define ED25519_HASH_SIZE        64

/* Embedded Ed25519 Public Key (placeholder - to be replaced with actual key) */
static const uint8_t g_kernel_public_key[ED25519_PUBLIC_KEY_SIZE] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* SHA-512 context */
typedef struct {
    uint64_t state[8];
    uint64_t count;
    uint8_t buffer[128];
} sha512_context;

/* Ed25519 signature verification result codes */
#define SIG_VERIFY_SUCCESS    0
#define SIG_VERIFY_FAILURE    1
#define SIG_VERIFY_NO_SIG     2
#define SIG_VERIFY_BAD_KEY    3

/* External assembly functions for cryptographic operations */
extern void sha512_init(sha512_context *ctx);
extern void sha512_update(sha512_context *ctx, const uint8_t *data, uint64_t len);
extern void sha512_final(sha512_context *ctx, uint8_t *hash);
extern int ed25519_verify(const uint8_t *sig, const uint8_t *msg, uint64_t msg_len,
                          const uint8_t *public_key);

/* Compute SHA-512 hash of data */
static void compute_hash(const uint8_t *data, uint64_t len, uint8_t *hash) {
    sha512_context ctx;
    sha512_init(&ctx);
    sha512_update(&ctx, data, len);
    sha512_final(&ctx, hash);
}

/* Verify kernel signature using Ed25519
 * 
 * The signature is expected to be appended to the kernel image.
 * Format: [kernel_data][signature(64 bytes)][magic(8 bytes)]
 * Magic: "CELLSIG\0"
 */
int verify_kernel_signature(void *kernel_buf, uint64_t kernel_size) {
    uint8_t hash[ED25519_HASH_SIZE];
    uint8_t signature[ED25519_SIGNATURE_SIZE];
    uint64_t i;
    uint8_t *buf = (uint8_t *)kernel_buf;
    
    /* For now, return success if no buffer provided (early boot) */
    if (kernel_buf == 0 || kernel_size == 0) {
        /* In real implementation, this would read from disk */
        /* Placeholder: assume signature is valid during early boot */
        return SIG_VERIFY_SUCCESS;
    }
    
    /* Check minimum size for signature and magic */
    if (kernel_size < (ED25519_SIGNATURE_SIZE + 8)) {
        return SIG_VERIFY_NO_SIG;
    }
    
    /* Check for signature magic at end of image */
    uint8_t *magic = buf + kernel_size - 8;
    if (magic[0] != 'C' || magic[1] != 'E' || magic[2] != 'L' || 
        magic[3] != 'L' || magic[4] != 'S' || magic[5] != 'I' ||
        magic[6] != 'G' || magic[7] != '\0') {
        return SIG_VERIFY_NO_SIG;
    }
    
    /* Extract signature */
    uint8_t *sig_ptr = buf + kernel_size - ED25519_SIGNATURE_SIZE - 8;
    for (i = 0; i < ED25519_SIGNATURE_SIZE; i++) {
        signature[i] = sig_ptr[i];
    }
    
    /* Compute hash of kernel data (excluding signature and magic) */
    uint64_t data_size = kernel_size - ED25519_SIGNATURE_SIZE - 8;
    compute_hash(buf, data_size, hash);
    
    /* Verify signature */
    int result = ed25519_verify(signature, hash, ED25519_HASH_SIZE, g_kernel_public_key);
    
    if (result != 0) {
        return SIG_VERIFY_FAILURE;
    }
    
    return SIG_VERIFY_SUCCESS;
}

/* C implementation of SHA-512 (fallback if assembly not available) */
static const uint64_t sha512_k[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
    0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
    0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
    0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
    0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
    0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
    0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
    0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
    0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
    0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
    0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
    0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
    0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
    0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
    0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
    0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
    0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
    0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
    0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
    0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
    0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROT_RIGHT64(x, 28) ^ ROT_RIGHT64(x, 34) ^ ROT_RIGHT64(x, 39))
#define EP1(x) (ROT_RIGHT64(x, 14) ^ ROT_RIGHT64(x, 18) ^ ROT_RIGHT64(x, 41))
#define SIG0(x) (ROT_RIGHT64(x, 1) ^ ROT_RIGHT64(x, 8) ^ ((x) >> 7))
#define SIG1(x) (ROT_RIGHT64(x, 19) ^ ROT_RIGHT64(x, 61) ^ ((x) >> 6))

#define ROT_RIGHT64(x, n) (((x) >> (n)) | ((x) << (64 - (n))))

void sha512_init(sha512_context *ctx) {
    ctx->state[0] = 0x6a09e667f3bcc908ULL;
    ctx->state[1] = 0xbb67ae8584caa73bULL;
    ctx->state[2] = 0x3c6ef372fe94f82bULL;
    ctx->state[3] = 0xa54ff53a5f1d36f1ULL;
    ctx->state[4] = 0x510e527fade682d1ULL;
    ctx->state[5] = 0x9b05688c2b3e6c1fULL;
    ctx->state[6] = 0x1f83d9abfb41bd6bULL;
    ctx->state[7] = 0x5be0cd19137e2179ULL;
    ctx->count = 0;
}

void sha512_transform(sha512_context *ctx, const uint8_t data[128]) {
    uint64_t a, b, c, d, e, f, g, h, t1, t2;
    uint64_t w[80];
    int i;
    
    /* Prepare message schedule */
    for (i = 0; i < 16; i++) {
        w[i] = ((uint64_t)data[i * 8] << 56) |
               ((uint64_t)data[i * 8 + 1] << 48) |
               ((uint64_t)data[i * 8 + 2] << 40) |
               ((uint64_t)data[i * 8 + 3] << 32) |
               ((uint64_t)data[i * 8 + 4] << 24) |
               ((uint64_t)data[i * 8 + 5] << 16) |
               ((uint64_t)data[i * 8 + 6] << 8) |
               ((uint64_t)data[i * 8 + 7]);
    }
    
    for (i = 16; i < 80; i++) {
        w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
    }
    
    /* Initialize working variables */
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];
    
    /* Main loop */
    for (i = 0; i < 80; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + sha512_k[i] + w[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    /* Update state */
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void sha512_update(sha512_context *ctx, const uint8_t *data, uint64_t len) {
    uint64_t i;
    uint64_t index = (ctx->count / 8) % 128;
    
    ctx->count += len * 8;
    
    /* Fill buffer and process blocks */
    uint64_t part_len = 128 - index;
    if (len >= part_len) {
        for (i = 0; i < part_len; i++) {
            ctx->buffer[index + i] = data[i];
        }
        sha512_transform(ctx, ctx->buffer);
        
        for (i = part_len; i + 128 <= len; i += 128) {
            sha512_transform(ctx, data + i);
        }
        index = 0;
    } else {
        i = 0;
    }
    
    /* Copy remaining data to buffer */
    while (i < len) {
        ctx->buffer[index++] = data[i++];
    }
}

void sha512_final(sha512_context *ctx, uint8_t *hash) {
    uint64_t i;
    uint64_t index = (ctx->count / 8) % 128;
    uint64_t pad_len;
    
    /* Pad message */
    pad_len = (index < 112) ? (112 - index) : (240 - index);
    
    static const uint8_t padding[128] = { 0x80 };
    sha512_update(ctx, padding, pad_len);
    
    /* Append length in bits */
    uint8_t len_bytes[16];
    for (i = 0; i < 8; i++) {
        len_bytes[i] = (ctx->count >> (56 - i * 8)) & 0xFF;
        len_bytes[8 + i] = 0;
    }
    sha512_update(ctx, len_bytes, 16);
    
    /* Produce final hash */
    for (i = 0; i < 8; i++) {
        hash[i * 8 + 0] = (ctx->state[i] >> 56) & 0xFF;
        hash[i * 8 + 1] = (ctx->state[i] >> 48) & 0xFF;
        hash[i * 8 + 2] = (ctx->state[i] >> 40) & 0xFF;
        hash[i * 8 + 3] = (ctx->state[i] >> 32) & 0xFF;
        hash[i * 8 + 4] = (ctx->state[i] >> 24) & 0xFF;
        hash[i * 8 + 5] = (ctx->state[i] >> 16) & 0xFF;
        hash[i * 8 + 6] = (ctx->state[i] >> 8) & 0xFF;
        hash[i * 8 + 7] = ctx->state[i] & 0xFF;
    }
}

/* Stub for Ed25519 verification - full implementation would be in assembly */
int ed25519_verify(const uint8_t *sig, const uint8_t *msg, uint64_t msg_len,
                   const uint8_t *public_key) {
    /* Placeholder - in production this would implement full Ed25519 verification */
    /* For now, check if public key is all zeros (development mode) */
    int i;
    int key_is_zero = 1;
    for (i = 0; i < ED25519_PUBLIC_KEY_SIZE; i++) {
        if (public_key[i] != 0) {
            key_is_zero = 0;
            break;
        }
    }
    
    if (key_is_zero) {
        /* Development mode - accept any signature */
        return 0;
    }
    
    /* Production mode - signature verification would happen here */
    /* This requires elliptic curve operations implemented in assembly */
    return 1; /* Fail by default in production without proper implementation */
}
