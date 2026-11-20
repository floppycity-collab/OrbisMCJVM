/**
 * @file security.h
 * @brief OrbisJVM Security API
 *
 * Provides cryptographic operations, secure random, and security utilities.
 */

#ifndef ORBISJVM_SECURITY_H
#define ORBISJVM_SECURITY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Hash algorithm
 */
typedef enum {
    ORBISJVM_HASH_MD5      = 0,
    ORBISJVM_HASH_SHA1     = 1,
    ORBISJVM_HASH_SHA224   = 2,
    ORBISJVM_HASH_SHA256   = 3,
    ORBISJVM_HASH_SHA384   = 4,
    ORBISJVM_HASH_SHA512   = 5,
    ORBISJVM_HASH_SHA3_224 = 6,
    ORBISJVM_HASH_SHA3_256 = 7,
    ORBISJVM_HASH_SHA3_384 = 8,
    ORBISJVM_HASH_SHA3_512 = 9,
} orbisjvm_hash_algo_t;

/**
 * @brief Hash context
 */
typedef struct orbisjvm_hash_ctx* orbisjvm_hash_ctx_t;

/**
 * @brief Create hash context
 *
 * @param[out] ctx Output hash context
 * @param algo Hash algorithm
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_hash_create(orbisjvm_hash_ctx_t* ctx, orbisjvm_hash_algo_t algo);

/**
 * @brief Destroy hash context
 *
 * @param ctx Hash context
 */
void orbisjvm_hash_destroy(orbisjvm_hash_ctx_t ctx);

/**
 * @brief Initialize hash context
 *
 * @param ctx Hash context
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_hash_init(orbisjvm_hash_ctx_t ctx);

/**
 * @brief Update hash with data
 *
 * @param ctx Hash context
 * @param data Input data
 * @param len Data length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_hash_update(orbisjvm_hash_ctx_t ctx, const void* data, size_t len);

/**
 * @brief Finalize hash
 *
 * @param ctx Hash context
 * @param[out] out Output hash buffer
 * @param out_len Output buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_hash_final(orbisjvm_hash_ctx_t ctx, void* out, size_t out_len);

/**
 * @brief Get hash output size for algorithm
 *
 * @param algo Hash algorithm
 * @return Hash output size in bytes
 */
size_t orbisjvm_hash_get_output_size(orbisjvm_hash_algo_t algo);

/**
 * @brief Compute hash in one call
 *
 * @param algo Hash algorithm
 * @param data Input data
 * @param len Data length
 * @param[out] out Output hash buffer
 * @param out_len Output buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_hash_compute(orbisjvm_hash_algo_t algo, const void* data, size_t len,
                          void* out, size_t out_len);

/**
 * @brief HMAC context
 */
typedef struct orbisjvm_hmac_ctx* orbisjvm_hmac_ctx_t;

/**
 * @brief Create HMAC context
 *
 * @param[out] ctx Output HMAC context
 * @param algo Hash algorithm
 * @param key HMAC key
 * @param key_len Key length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_hmac_create(orbisjvm_hmac_ctx_t* ctx, orbisjvm_hash_algo_t algo,
                         const void* key, size_t key_len);

/**
 * @brief Destroy HMAC context
 *
 * @param ctx HMAC context
 */
void orbisjvm_hmac_destroy(orbisjvm_hmac_ctx_t ctx);

/**
 * @brief Initialize HMAC context
 *
 * @param ctx HMAC context
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_hmac_init(orbisjvm_hmac_ctx_t ctx);

/**
 * @brief Update HMAC with data
 *
 * @param ctx HMAC context
 * @param data Input data
 * @param len Data length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_hmac_update(orbisjvm_hmac_ctx_t ctx, const void* data, size_t len);

/**
 * @brief Finalize HMAC
 *
 * @param ctx HMAC context
 * @param[out] out Output HMAC buffer
 * @param out_len Output buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_hmac_final(orbisjvm_hmac_ctx_t ctx, void* out, size_t out_len);

/**
 * @brief Compute HMAC in one call
 *
 * @param algo Hash algorithm
 * @param key HMAC key
 * @param key_len Key length
 * @param data Input data
 * @param len Data length
 * @param[out] out Output HMAC buffer
 * @param out_len Output buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_hmac_compute(orbisjvm_hash_algo_t algo, const void* key, size_t key_len,
                          const void* data, size_t len, void* out, size_t out_len);

/**
 * @brief Cipher algorithm
 */
typedef enum {
    ORBISJVM_CIPHER_AES_128_ECB = 0,
    ORBISJVM_CIPHER_AES_192_ECB = 1,
    ORBISJVM_CIPHER_AES_256_ECB = 2,
    ORBISJVM_CIPHER_AES_128_CBC = 3,
    ORBISJVM_CIPHER_AES_192_CBC = 4,
    ORBISJVM_CIPHER_AES_256_CBC = 5,
    ORBISJVM_CIPHER_AES_128_CTR = 6,
    ORBISJVM_CIPHER_AES_192_CTR = 7,
    ORBISJVM_CIPHER_AES_256_CTR = 8,
    ORBISJVM_CIPHER_AES_128_GCM = 9,
    ORBISJVM_CIPHER_AES_192_GCM = 10,
    ORBISJVM_CIPHER_AES_256_GCM = 11,
    ORBISJVM_CIPHER_CHACHA20_POLY1305 = 12,
} orbisjvm_cipher_algo_t;

/**
 * @brief Cipher context
 */
typedef struct orbisjvm_cipher_ctx* orbisjvm_cipher_ctx_t;

/**
 * @brief Cipher operation
 */
typedef enum {
    ORBISJVM_CIPHER_ENCRYPT = 0,
    ORBISJVM_CIPHER_DECRYPT = 1,
} orbisjvm_cipher_op_t;

/**
 * @brief Create cipher context
 *
 * @param[out] ctx Output cipher context
 * @param algo Cipher algorithm
 * @param op Encrypt or decrypt
 * @param key Key
 * @param key_len Key length
 * @param iv IV (or NULL for ECB)
 * @param iv_len IV length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_cipher_create(orbisjvm_cipher_ctx_t* ctx, orbisjvm_cipher_algo_t algo,
                           orbisjvm_cipher_op_t op, const void* key, size_t key_len,
                           const void* iv, size_t iv_len);

/**
 * @brief Destroy cipher context
 *
 * @param ctx Cipher context
 */
void orbisjvm_cipher_destroy(orbisjvm_cipher_ctx_t ctx);

/**
 * @brief Process data (encrypt/decrypt)
 *
 * @param ctx Cipher context
 * @param in Input data
 * @param in_len Input length
 * @param[out] out Output buffer
 * @param out_len Output buffer length
 * @param[out] out_written Bytes written
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_cipher_process(orbisjvm_cipher_ctx_t ctx, const void* in, size_t in_len,
                            void* out, size_t out_len, size_t* out_written);

/**
 * @brief Finalize cipher
 *
 * @param ctx Cipher context
 * @param[out] out Output buffer
 * @param out_len Output buffer length
 * @param[out] out_written Bytes written
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_cipher_final(orbisjvm_cipher_ctx_t ctx, void* out, size_t out_len, size_t* out_written);

/**
 * @brief Generate cryptographically secure random bytes
 *
 * @param[out] buffer Output buffer
 * @param size Buffer size
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_secure_random(void* buffer, size_t size);

/**
 * @brief Generate random UUID
 *
 * @param[out] uuid Output buffer (37 bytes)
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_random_uuid(char* uuid);

/**
 * @brief Generate random bytes (non-cryptographic, fast)
 *
 * @param[out] buffer Output buffer
 * @param size Buffer size
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_random_bytes(void* buffer, size_t size);

/**
 * @brief Secure memory zero (prevents compiler optimization)
 *
 * @param ptr Memory to zero
 * @param size Size in bytes
 */
void orbisjvm_secure_zero(void* ptr, size_t size);

/**
 * @brief Constant-time memory comparison
 *
 * @param a First buffer
 * @param b Second buffer
 * @param size Size in bytes
 * @return 0 if equal, non-zero if different
 */
int orbisjvm_constant_time_eq(const void* a, const void* b, size_t size);

/**
 * @brief Constant-time memory copy
 *
 * @param dest Destination
 * @param src Source
 * @param size Size in bytes
 */
void orbisjvm_constant_time_copy(void* dest, const void* src, size_t size);

/**
 * @brief Path traversal check
 *
 * @param base Base directory (must be absolute)
 * @param path Path to check (may be relative)
 * @return 1 if path is within base, 0 if traversal detected, negative error code on failure
 */
int orbisjvm_path_is_safe(const char* base, const char* path);

/**
 * @brief Sanitize path (remove .. and . components, resolve symlinks)
 *
 * @param path Input path
 * @param[out] buffer Output buffer
 * @param buflen Buffer length
 * @return 0 on success, negative error code on failure
 */
int orbisjvm_path_sanitize(const char* path, char* buffer, size_t buflen);

/**
 * @brief Validate filename (no path separators, no null bytes, reasonable length)
 *
 * @param filename Filename to validate
 * @return 1 if valid, 0 if invalid
 */
int orbisjvm_filename_valid(const char* filename);

/**
 * @brief Security capabilities
 */
typedef struct {
    int has_aes_ni;         ///< Hardware AES acceleration
    int has_rdrand;         ///< Hardware random number generator
    int has_rdseed;         ///< Hardware random seed
    int has_sha_ext;        ///< Hardware SHA acceleration
    int secure_boot;        ///< Secure boot enabled
    int memory_encryption;  ///< Memory encryption active
} orbisjvm_security_caps_t;

/**
 * @brief Get security capabilities
 *
 * @param[out] caps Security capabilities
 */
void orbisjvm_get_security_caps(orbisjvm_security_caps_t* caps);

#ifdef __cplusplus
}
#endif

#endif // ORBISJVM_SECURITY_H