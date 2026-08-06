#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define CHUNK_SIZE 4096
#define BLOCK_SIZE_BYTES 64   /* 512 bits = 64 bytes */
#define PADDING_TARGET_BYTES 56 /* 448 bits = 56 bytes */

/* Bitwise Rotation and Shift Helper Macros */
#define ROTR(x, n) (((uint32_t)(x) >> (n)) | ((uint32_t)(x) << (32 - (n))))
#define SHR(x, n)  ((uint32_t)(x) >> (n))

/* SHA-256 Bitwise Functions */
#define SIGMA0(x)     (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3))     /* sig0 / σ0 */
#define SIGMA1(x)     (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10))   /* sig1 / σ1 */
#define CAP_SIGMA0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))   /* SIG0 / Σ0 */
#define CAP_SIGMA1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))   /* SIG1 / Σ1 */

#define CH(e, f, g)   (((e) & (f)) ^ (~(e) & (g)))               /* Choose */
#define MAJ(a, b, c)  (((a) & (b)) ^ ((a) & (c)) ^ ((b) & (c)))   /* Majority */

/**
 * SHA-256 Initial Hash State Values (H0 - H7)
 * First 32 bits of fractional parts of square roots of first 8 prime numbers (2..19)
 */
static const uint32_t SHA256_H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

/**
 * SHA-256 Round Constants (K0 to K63)
 * First 32 bits of fractional parts of cube roots of first 64 prime numbers (2..311)
 */
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/**
 * SHA-256 Context Structure
 */
typedef struct {
    uint32_t state[8];                /* 8 32-bit state registers (H0 - H7) */
    uint8_t buffer[BLOCK_SIZE_BYTES]; /* 64-byte working buffer */
    size_t buffer_len;               /* Current number of bytes in buffer (0 to 63) */
    uint64_t total_bits;             /* Total message length in bits */
    size_t block_count;              /* Count of 512-bit blocks processed */
} sha256_ctx_t;

/**
 * Initializes the SHA-256 context with H0-H7 state registers.
 */
void sha256_init(sha256_ctx_t *ctx) {
    for (int i = 0; i < 8; i++) {
        ctx->state[i] = SHA256_H0[i];
    }
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
    ctx->buffer_len = 0;
    ctx->total_bits = 0;
    ctx->block_count = 0;
}

/**
 * Transform function to process one 512-bit (64-byte) block M^(i).
 */
void sha256_transform_block(sha256_ctx_t *ctx, const uint8_t block[64]) {
    uint32_t W[64];

    /*Message Schedule Expansion (W0 ... W63) */
    for (int t = 0; t < 16; t++) {
        W[t] = ((uint32_t)block[t * 4] << 24) |
               ((uint32_t)block[t * 4 + 1] << 16) |
               ((uint32_t)block[t * 4 + 2] << 8) |
               ((uint32_t)block[t * 4 + 3]);
    }

    for (int t = 16; t < 64; t++) {
        W[t] = SIGMA1(W[t - 2]) + W[t - 7] + SIGMA0(W[t - 15]) + W[t - 16];
    }

    /* Initialize Working Variables */
    uint32_t a = ctx->state[0];
    uint32_t b = ctx->state[1];
    uint32_t c = ctx->state[2];
    uint32_t d = ctx->state[3];
    uint32_t e = ctx->state[4];
    uint32_t f = ctx->state[5];
    uint32_t g = ctx->state[6];
    uint32_t h = ctx->state[7];

    /* Run 64 Compression Rounds */
    for (int t = 0; t < 64; t++) {
        uint32_t T1 = h + CAP_SIGMA1(e) + CH(e, f, g) + K[t] + W[t];
        uint32_t T2 = CAP_SIGMA0(a) + MAJ(a, b, c);

        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    /* Step 4: Intermediate State Accumulation */
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;

    ctx->block_count++;
}

/**
 * Updates context with raw binary input bytes.
 */
void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, size_t len) {
    ctx->total_bits += ((uint64_t)len * 8);

    for (size_t i = 0; i < len; i++) {
        ctx->buffer[ctx->buffer_len++] = data[i];

        if (ctx->buffer_len == BLOCK_SIZE_BYTES) {
            sha256_transform_block(ctx, ctx->buffer);
            ctx->buffer_len = 0;
        }
    }
}

/**
 * Finalizes SHA-256 preprocessing and produces the 32-byte digest output.
 */
void sha256_final(sha256_ctx_t *ctx, uint8_t digest[32]) {
    /* 1. Append '1' bit (0x80 byte) */
    ctx->buffer[ctx->buffer_len++] = 0x80;

    /* 2. Zero-pad current block if length > 56 bytes */
    if (ctx->buffer_len > PADDING_TARGET_BYTES) {
        while (ctx->buffer_len < BLOCK_SIZE_BYTES) {
            ctx->buffer[ctx->buffer_len++] = 0x00;
        }
        sha256_transform_block(ctx, ctx->buffer);
        ctx->buffer_len = 0;
    }

    /* Zero-pad until byte 56 */
    while (ctx->buffer_len < PADDING_TARGET_BYTES) {
        ctx->buffer[ctx->buffer_len++] = 0x00;
    }

    /* 3. Append 64-bit big-endian original message bit length into bytes 56..63 */
    uint64_t bits = ctx->total_bits;
    ctx->buffer[56] = (uint8_t)(bits >> 56);
    ctx->buffer[57] = (uint8_t)(bits >> 48);
    ctx->buffer[58] = (uint8_t)(bits >> 40);
    ctx->buffer[59] = (uint8_t)(bits >> 32);
    ctx->buffer[60] = (uint8_t)(bits >> 24);
    ctx->buffer[61] = (uint8_t)(bits >> 16);
    ctx->buffer[62] = (uint8_t)(bits >> 8);
    ctx->buffer[63] = (uint8_t)(bits >> 0);

    sha256_transform_block(ctx, ctx->buffer);
    ctx->buffer_len = 0;

    /* Produce final 32-byte big-endian digest from state H0..H7 */
    for (int i = 0; i < 8; i++) {
        digest[i * 4]     = (uint8_t)(ctx->state[i] >> 24);
        digest[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        digest[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 8);
        digest[i * 4 + 3] = (uint8_t)(ctx->state[i] >> 0);
    }
}

/**
 * Computes SHA-256 checksum of a binary file.
 */
int sha256_file(const char *filepath, uint8_t digest[32]) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Error opening binary file");
        return -1;
    }

    sha256_ctx_t ctx;
    sha256_init(&ctx);

    uint8_t chunk[CHUNK_SIZE];
    size_t bytes_read = 0;

    while ((bytes_read = fread(chunk, 1, sizeof(chunk), file)) > 0) {
        sha256_update(&ctx, chunk, bytes_read);
    }

    if (ferror(file)) {
        fprintf(stderr, "Error reading binary file: %s\n", filepath);
        fclose(file);
        return -2;
    }

    fclose(file);
    sha256_final(&ctx, digest);
    return 0;
}

/**
 * Helper utility to convert a 32-byte digest to a 64-character hex string.
 */
void sha256_digest_to_hex(const uint8_t digest[32], char hex_output[65]) {
    for (int i = 0; i < 32; i++) {
        sprintf(hex_output + (i * 2), "%02x", digest[i]);
    }
    hex_output[64] = '\0';
}

/**
 * Helper function to strip leading/trailing quotes and newlines from input paths.
 */
static void sanitize_filepath(char *path) {
    size_t len = strlen(path);
    /* Remove trailing newlines / carriage returns */
    while (len > 0 && (path[len - 1] == '\n' || path[len - 1] == '\r')) {
        path[--len] = '\0';
    }

    /* Remove surrounding double quotes if user pasted quoted path */
    if (len >= 2 && path[0] == '"' && path[len - 1] == '"') {
        memmove(path, path + 1, len - 2);
        path[len - 2] = '\0';
    }
}

int main(int argc, char *argv[]) {
    char filepath[1024] = {0};

    if (argc >= 2) {
        strncpy(filepath, argv[1], sizeof(filepath) - 1);
    } else {
        printf("Enter the file path: ");
        if (fgets(filepath, sizeof(filepath), stdin) == NULL) {
            fprintf(stderr, "Error reading file path from input.\n");
            return 1;
        }
        sanitize_filepath(filepath);

        /* If user pressed Enter without typing a path, run test vectors */
        if (strlen(filepath) == 0) {
            printf("\nNo file path provided.\n");

            return 0;
        }
    }

    uint8_t digest[32];
    char hex_out[65];

    printf("Computing SHA-256 for file: %s\n", filepath);
    int status = sha256_file(filepath, digest);
    if (status == 0) {
        sha256_digest_to_hex(digest, hex_out);
        printf("SHA-256 Hash: %s\n", hex_out);
    } else {
        printf("Failed to compute SHA-256 hash. Error code: %d\n", status);
    }

    return status;
}
