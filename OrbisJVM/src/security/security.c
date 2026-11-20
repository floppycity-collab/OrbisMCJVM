/**
 * @file security.c
 * @brief OrbisJVM Security Implementation
 *
 * Implements cryptographic operations, secure random, and security utilities.
 */

#include <orbisjvm/security.h>
#include <orbisjvm/diagnostics.h>
#include <orbisjvm/memory.h>

#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/sha.h>

/* Hash context */
struct orbisjvm_hash_ctx {
    EVP_MD_CTX* ctx;
    orbisjvm_hash_algo_t algo;
};

/* HMAC context */
struct orbisjvm_hmac_ctx {
    HMAC_CTX* ctx;
    orbisjvm_hash_algo_t algo;
};

/* Cipher context */
struct orbisjvm_cipher_ctx {
    EVP_CIPHER_CTX* ctx;
    orbisjvm_cipher_algo_t algo;
    orbisjvm_cipher_op_t op;
};

/* Algorithm mapping */

static const EVP_MD* hash_algo_to_evp(orbisjvm_hash_algo_t algo) {
    switch (algo) {
        case ORBISJVM_HASH_MD5: return EVP_md5();
        case ORBISJVM_HASH_SHA1: return EVP_sha1();
        case ORBISJVM_HASH_SHA224: return EVP_sha224();
        case ORBISJVM_HASH_SHA256: return EVP_sha256();
        case ORBISJVM_HASH_SHA384: return EVP_sha384();
        case ORBISJVM_HASH_SHA512: return EVP_sha512();
        case ORBISJVM_HASH_SHA3_224: return EVP_sha3_224();
        case ORBISJVM_HASH_SHA3_256: return EVP_sha3_256();
        case ORBISJVM_HASH_SHA3_384: return EVP_sha3_384();
        case ORBISJVM_HASH_SHA3_512: return EVP_sha3_512();
        default: return NULL;
    }
}

static const EVP_CIPHER* cipher_algo_to_evp(orbisjvm_cipher_algo_t algo) {
    switch (algo) {
        case ORBISJVM_CIPHER_AES_128_ECB: return EVP_aes_128_ecb();
        case ORBISJVM_CIPHER_AES_192_ECB: return EVP_aes_192_ecb();
        case ORBISJVM_CIPHER_AES_256_ECB: return EVP_aes_256_ecb();
        case ORBISJVM_CIPHER_AES_128_CBC: return EVP_aes_128_cbc();
        case ORBISJVM_CIPHER_AES_192_CBC: return EVP_aes_192_cbc();
        case ORBISJVM_CIPHER_AES_256_CBC: return EVP_aes_256_cbc();
        case ORBISJVM_CIPHER_AES_128_CTR: return EVP_aes_128_ctr();
        case ORBISJVM_CIPHER_AES_192_CTR: return EVP_aes_192_ctr();
        case ORBISJVM_CIPHER_AES_256_CTR: return EVP_aes_256_ctr();
        case ORBISJVM_CIPHER_AES_128_GCM: return EVP_aes_128_gcm();
        case ORBISJVM_CIPHER_AES_192_GCM: return EVP_aes_192_gcm();
        case ORBISJVM_CIPHER_AES_256_GCM: return EVP_aes_256_gcm();
        case ORBISJVM_CIPHER_CHACHA20_POLY1305: return EVP_chacha20_poly1305();
        default: return NULL;
    }
}

/**
 * @brief Initialize security subsystem
 */
int orbisjvm_security_init(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_SECURITY, "Initializing security subsystem");
    OpenSSL_add_all_algorithms();
    return 0;
}

/**
 * @brief Shutdown security subsystem
 */
void orbisjvm_security_shutdown(void) {
    orbisjvm_log_info(ORBISJVM_LOG_CAT_SECURITY, "Shutting down security subsystem");
    EVP_cleanup();
}

/* Hash functions */

int orbisjvm_hash_create(orbisjvm_hash_ctx_t* ctx, orbisjvm_hash_algo_t algo) {
    if (!ctx) return -1;

    const EVP_MD* md = hash_algo_to_evp(algo);
    if (!md) return -EINVAL;

    orbisjvm_hash_ctx_t h = orbisjvm_malloc(sizeof(struct orbisjvm_hash_ctx), ORBISJVM_MEM_ZERO);
    if (!h) return -ENOMEM;

    h->ctx = EVP_MD_CTX_new();
    if (!h->ctx) {
        orbisjvm_free(h);
        return -ENOMEM;
    }

    h->algo = algo;

    int ret = EVP_DigestInit_ex(h->ctx, md, NULL);
    if (ret != 1) {
        EVP_MD_CTX_free(h->ctx);
        orbisjvm_free(h);
        return -1;
    }

    *ctx = h;
    return 0;
}

void orbisjvm_hash_destroy(orbisjvm_hash_ctx_t ctx) {
    if (!ctx) return;
    EVP_MD_CTX_free(ctx->ctx);
    orbisjvm_free(ctx);
}

int orbisjvm_hash_init(orbisjvm_hash_ctx_t ctx) {
    if (!ctx) return -1;
    const EVP_MD* md = hash_algo_to_evp(ctx->algo);
    return EVP_DigestInit_ex(ctx->ctx, md, NULL) == 1 ? 0 : -1;
}

int orbisjvm_hash_update(orbisjvm_hash_ctx_t ctx, const void* data, size_t len) {
    if (!ctx || !data) return -1;
    return EVP_DigestUpdate(ctx->ctx, data, len) == 1 ? 0 : -1;
}

int orbisjvm_hash_final(orbisjvm_hash_ctx_t ctx, void* out, size_t out_len) {
    if (!ctx || !out) return -1;

    unsigned int md_len;
    int ret = EVP_DigestFinal_ex(ctx->ctx, out, &md_len);
    if (ret != 1) return -1;

    if (out_len < md_len) return -1;
    return 0;
}

size_t orbisjvm_hash_get_output_size(orbisjvm_hash_algo_t algo) {
    const EVP_MD* md = hash_algo_to_evp(algo);
    return md ? EVP_MD_size(md) : 0;
}

int orbisjvm_hash_compute(orbisjvm_hash_algo_t algo, const void* data, size_t len,
                          void* out, size_t out_len) {
    orbisjvm_hash_ctx_t ctx;
    int ret = orbisjvm_hash_create(&ctx, algo);
    if (ret < 0) return ret;

    ret = orbisjvm_hash_update(ctx, data, len);
    if (ret == 0) {
        ret = orbisjvm_hash_final(ctx, out, out_len);
    }

    orbisjvm_hash_destroy(ctx);
    return ret;
}

/* HMAC functions */

int orbisjvm_hmac_create(orbisjvm_hmac_ctx_t* ctx, orbisjvm_hash_algo_t algo,
                         const void* key, size_t key_len) {
    if (!ctx || !key) return -1;

    const EVP_MD* md = hash_algo_to_evp(algo);
    if (!md) return -EINVAL;

    orbisjvm_hmac_ctx_t h = orbisjvm_malloc(sizeof(struct orbisjvm_hmac_ctx), ORBISJVM_MEM_ZERO);
    if (!h) return -ENOMEM;

    h->ctx = HMAC_CTX_new();
    if (!h->ctx) {
        orbisjvm_free(h);
        return -ENOMEM;
    }

    h->algo = algo;

    int ret = HMAC_Init_ex(h->ctx, key, key_len, md, NULL);
    if (ret != 1) {
        HMAC_CTX_free(h->ctx);
        orbisjvm_free(h);
        return -1;
    }

    *ctx = h;
    return 0;
}

void orbisjvm_hmac_destroy(orbisjvm_hmac_ctx_t ctx) {
    if (!ctx) return;
    HMAC_CTX_free(ctx->ctx);
    orbisjvm_free(ctx);
}

int orbisjvm_hmac_init(orbisjvm_hmac_ctx_t ctx) {
    if (!ctx) return -1;
    const EVP_MD* md = hash_algo_to_evp(ctx->algo);
    return HMAC_Init_ex(ctx->ctx, NULL, 0, md, NULL) == 1 ? 0 : -1;
}

int orbisjvm_hmac_update(orbisjvm_hmac_ctx_t ctx, const void* data, size_t len) {
    if (!ctx || !data) return -1;
    return HMAC_Update(ctx->ctx, data, len) == 1 ? 0 : -1;
}

int orbisjvm_hmac_final(orbisjvm_hmac_ctx_t ctx, void* out, size_t out_len) {
    if (!ctx || !out) return -1;

    unsigned int md_len;
    int ret = HMAC_Final(ctx->ctx, out, &md_len);
    if (ret != 1) return -1;

    if (out_len < md_len) return -1;
    return 0;
}

int orbisjvm_hmac_compute(orbisjvm_hash_algo_t algo, const void* key, size_t key_len,
                          const void* data, size_t len, void* out, size_t out_len) {
    orbisjvm_hmac_ctx_t ctx;
    int ret = orbisjvm_hmac_create(&ctx, algo, key, key_len);
    if (ret < 0) return ret;

    ret = orbisjvm_hmac_update(ctx, data, len);
    if (ret == 0) {
        ret = orbisjvm_hmac_final(ctx, out, out_len);
    }

    orbisjvm_hmac_destroy(ctx);
    return ret;
}

/* Cipher functions */

int orbisjvm_cipher_create(orbisjvm_cipher_ctx_t* ctx, orbisjvm_cipher_algo_t algo,
                           orbisjvm_cipher_op_t op, const void* key, size_t key_len,
                           const void* iv, size_t iv_len) {
    if (!ctx || !key) return -1;

    const EVP_CIPHER* cipher = cipher_algo_to_evp(algo);
    if (!cipher) return -EINVAL;

    orbisjvm_cipher_ctx_t c = orbisjvm_malloc(sizeof(struct orbisjvm_cipher_ctx), ORBISJVM_MEM_ZERO);
    if (!c) return -ENOMEM;

    c->ctx = EVP_CIPHER_CTX_new();
    if (!c->ctx) {
        orbisjvm_free(c);
        return -ENOMEM;
    }

    c->algo = algo;
    c->op = op;

    int ret = EVP_CipherInit_ex(c->ctx, cipher, NULL, key, iv, op == ORBISJVM_CIPHER_ENCRYPT ? 1 : 0);
    if (ret != 1) {
        EVP_CIPHER_CTX_free(c->ctx);
        orbisjvm_free(c);
        return -1;
    }

    *ctx = c;
    return 0;
}

void orbisjvm_cipher_destroy(orbisjvm_cipher_ctx_t ctx) {
    if (!ctx) return;
    EVP_CIPHER_CTX_free(ctx->ctx);
    orbisjvm_free(ctx);
}

int orbisjvm_cipher_process(orbisjvm_cipher_ctx_t ctx, const void* in, size_t in_len,
                            void* out, size_t out_len, size_t* out_written) {
    if (!ctx || !in || !out) return -1;

    int len;
    int ret = EVP_CipherUpdate(ctx->ctx, out, &len, in, in_len);
    if (ret != 1) return -1;

    if (out_written) *out_written = len;
    return 0;
}

int orbisjvm_cipher_final(orbisjvm_cipher_ctx_t ctx, void* out, size_t out_len, size_t* out_written) {
    if (!ctx || !out) return -1;

    int len;
    int ret = EVP_CipherFinal_ex(ctx->ctx, out, &len);
    if (ret != 1) return -1;

    if (out_written) *out_written = len;
    return 0;
}

/* Random functions */

int orbisjvm_secure_random(void* buffer, size_t size) {
    if (!buffer || size == 0) return -1;
    return RAND_bytes(buffer, size) == 1 ? 0 : -1;
}

int orbisjvm_random_uuid(char* uuid) {
    if (!uuid) return -1;

    uint8_t bytes[16];
    int ret = orbisjvm_secure_random(bytes, 16);
    if (ret < 0) return ret;

    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    snprintf(uuid, 37,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0], bytes[1], bytes[2], bytes[3],
        bytes[4], bytes[5], bytes[6], bytes[7],
        bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);

    return 0;
}

int orbisjvm_random_bytes(void* buffer, size_t size) {
    if (!buffer || size == 0) return -1;
    return RAND_bytes(buffer, size) == 1 ? 0 : -1;
}

/* Secure memory operations */

void orbisjvm_secure_zero(void* ptr, size_t size) {
    if (!ptr) return;
    OPENSSL_cleanse(ptr, size);
}

int orbisjvm_constant_time_eq(const void* a, const void* b, size_t size) {
    if (!a || !b) return -1;
    return CRYPTO_memcmp(a, b, size);
}

void orbisjvm_constant_time_copy(void* dest, const void* src, size_t size) {
    if (!dest || !src) return;
    /* Constant-time copy not directly available, use memcpy with barrier */
    memcpy(dest, src, size);
    orbisjvm_memory_barrier();
}

/* Path validation */

int orbisjvm_path_is_safe(const char* base, const char* path) {
    if (!base || !path) return -1;

    char real_base[512];
    char real_path[512];

    if (!realpath(base, real_base)) return -1;
    if (!realpath(path, real_path)) return -1;

    size_t base_len = strlen(real_base);
    if (strncmp(real_path, real_base, base_len) != 0) return 0;

    if (real_path[base_len] != '\0' && real_path[base_len] != '/') return 0;

    return 1;
}

int orbisjvm_path_sanitize(const char* path, char* buffer, size_t buflen) {
    if (!path || !buffer || buflen == 0) return -1;

    char* resolved = realpath(path, buffer);
    if (!resolved) return -1;

    return 0;
}

int orbisjvm_filename_valid(const char* filename) {
    if (!filename) return 0;

    size_t len = strlen(filename);
    if (len == 0 || len > 255) return 0;

    for (size_t i = 0; i < len; i++) {
        char c = filename[i];
        if (c == '/' || c == '\\' || c == '\0' || c < 32 || c > 126) {
            return 0;
        }
    }

    if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) return 0;

    return 1;
}

/* Security capabilities */

void orbisjvm_get_security_caps(orbisjvm_security_caps_t* caps) {
    if (!caps) return;

    caps->has_aes_ni = 1;  /* PS4 CPU supports AES-NI */
    caps->has_rdrand = 1;  /* PS4 CPU supports RDRAND */
    caps->has_rdseed = 0;  /* Not available on Jaguar */
    caps->has_sha_ext = 0; /* No SHA extensions */
    caps->secure_boot = 1; /* PS4 has secure boot */
    caps->memory_encryption = 1;  /* PS4 encrypts memory */
}