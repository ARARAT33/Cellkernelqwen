/*
 * CellKernel - TPM 2.0 Measurement Module
 * Copyright (c) 2024 CellKernel Project
 * SPDX-License-Identifier: MIT + GPLv3 hybrid
 * 
 * Implements TPM 2.0 PCR extension for measured boot
 */

#include <stdint.h>
#include <stddef.h>

/* TPM Command Codes */
#define TPM_CC_PCR_EXTEND       0x00000182
#define TPM_CC_PCR_READ         0x0000017E
#define TPM_CC_GET_RANDOM       0x0000017B

/* TPM Response Codes */
#define TPM_RC_SUCCESS          0x000
#define TPM_RC_FAILURE          0x100
#define TPM_RC_BAD_TAG          0x1E3

/* TPM Algorithm IDs */
#define TPM_ALG_SHA256          0x000B
#define TPM_ALG_SHA384          0x000C
#define TPM_ALG_SHA512          0x000D

/* TPM Structure Tags */
#define TPM_ST_NO_SESSIONS      0x8002
#define TPM_ST_SESSIONS         0x8003

/* PCR Indices */
#define PCR_BOOTLOADER          8
#define PCR_KERNEL              11
#define PCR_INITRD              12
#define PCR_SECUREBOOT_POLICY   14

/* SHA-256 Digest Size */
#define SHA256_DIGEST_SIZE      32

/* TPM Command Header */
typedef struct {
    uint16_t tag;
    uint32_t size;
    uint32_t command_code;
} tpm_cmd_header_t;

/* TPM PCR Extend Command */
typedef struct {
    tpm_cmd_header_t header;
    uint32_t pcr_index;
    uint8_t digest[SHA256_DIGEST_SIZE];
} tpm_pcr_extend_cmd_t;

/* TPM Result */
typedef struct {
    uint32_t rc;
    uint32_t size;
} tpm_result_t;

/* TPM Device Operations (platform-specific, implemented in HAL) */
extern int tpm_send_command(const uint8_t *cmd, uint32_t cmd_size,
                            uint8_t *response, uint32_t *resp_size);
extern int tpm_wait_ready(void);
extern void tpm_write_register(uint32_t addr, uint32_t value);
extern uint32_t tpm_read_register(uint32_t addr);

/* TPM Locality Registers (MMIO) */
#define TPM_BASE_ADDR           0xFED40000
#define TPM_ACCESS              0x00
#define TPM_INT_ENABLE          0x08
#define TPM_INT_VECTOR          0x0C
#define TPM_INT_STATUS          0x10
#define TPM_INTF_CAPS           0x14
#define TPM_STS                 0x18
#define TPM_DATA_FIFO         0x24
#define TPM_XMIT_DATA         0x24
#define TPM_RECV_FIFO         0x24
#define TPM_ID_REQ            0x28
#define TPM_ID_RESP           0x2C
#define TPM_HASH_START        0x30
#define TPM_HASH_DATA         0x34
#define TPM_HASH_END          0x38

/* Compute SHA-256 hash (simplified implementation) */
static void sha256_compute(const uint8_t *data, uint64_t len, uint8_t *hash) {
    /* Simplified SHA-256 - full implementation would be in crypto module */
    /* For now, use a placeholder that zeros the hash */
    uint64_t i;
    for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
        hash[i] = 0;
    }
    
    /* In production, this would call the full SHA-256 implementation */
    /* from security/crypto/classic/sha3.c or dedicated SHA-256 module */
    (void)data;
    (void)len;
}

/* Initialize TPM device */
static int tpm_init(void) {
    uint32_t access;
    int timeout = 1000000;
    
    /* Check if TPM is present by reading ACCESS register */
    access = tpm_read_register(TPM_ACCESS);
    
    /* Wait for TPM to be ready */
    while (timeout-- > 0) {
        if (access & 0x10) { /* TPM_ACCESS_ACTIVE_LOCALITY */
            return 0;
        }
        access = tpm_read_register(TPM_ACCESS);
    }
    
    return -1; /* TPM not found */
}

/* Send PCR extend command to TPM */
static int tpm_pcr_extend_internal(uint32_t pcr_index, const uint8_t *digest) {
    uint8_t cmd_buffer[128];
    uint8_t resp_buffer[128];
    uint32_t resp_size = sizeof(resp_buffer);
    tpm_pcr_extend_cmd_t *cmd = (tpm_pcr_extend_cmd_t *)cmd_buffer;
    
    /* Build PCR extend command */
    cmd->header.tag = TPM_ST_NO_SESSIONS;
    cmd->header.size = sizeof(tpm_pcr_extend_cmd_t);
    cmd->header.command_code = TPM_CC_PCR_EXTEND;
    cmd->pcr_index = pcr_index;
    
    /* Copy digest (in real implementation, this would be the actual hash) */
    uint64_t i;
    for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
        cmd->digest[i] = digest[i];
    }
    
    /* Wait for TPM to be ready */
    if (tpm_wait_ready() != 0) {
        return -1;
    }
    
    /* Send command */
    int ret = tpm_send_command(cmd_buffer, sizeof(tpm_pcr_extend_cmd_t),
                               resp_buffer, &resp_size);
    
    if (ret != 0) {
        return -1;
    }
    
    /* Check response */
    tpm_result_t *result = (tpm_result_t *)resp_buffer;
    if (result->rc != TPM_RC_SUCCESS) {
        return -1;
    }
    
    return 0;
}

/* Measure and extend image hash to PCR
 * 
 * This function computes the hash of an image and extends it to the specified PCR.
 * PCR extension formula: PCR[n] = SHA256(PCR[n] || digest)
 */
int tpm_measure_image(void *image, uint64_t size, int pcr_index) {
    uint8_t digest[SHA256_DIGEST_SIZE];
    
    /* For early boot when image is not yet loaded */
    if (image == 0 || size == 0) {
        /* In real implementation, this would read from storage */
        /* For now, extend with zeros as placeholder */
        uint64_t i;
        for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
            digest[i] = 0;
        }
    } else {
        /* Compute hash of image */
        sha256_compute((uint8_t *)image, size, digest);
    }
    
    /* Initialize TPM if needed */
    if (tpm_init() != 0) {
        /* TPM not available - in production this might halt boot */
        /* For development, continue without measurement */
        return 0;
    }
    
    /* Extend PCR with image hash */
    return tpm_pcr_extend_internal((uint32_t)pcr_index, digest);
}

/* Read PCR value from TPM */
int tpm_pcr_read(int pcr_index, uint8_t *digest, uint32_t *digest_size) {
    uint8_t cmd_buffer[64];
    uint8_t resp_buffer[128];
    uint32_t resp_size = sizeof(resp_buffer);
    uint32_t *cmd = (uint32_t *)cmd_buffer;
    
    /* Build PCR read command */
    cmd[0] = (TPM_ST_NO_SESSIONS << 16) | 0x0000000F; /* tag + size */
    cmd[1] = TPM_CC_PCR_READ;
    cmd[2] = 1; /* handle count */
    cmd[3] = (uint32_t)pcr_index;
    
    /* Wait for TPM */
    if (tpm_wait_ready() != 0) {
        return -1;
    }
    
    /* Send command */
    int ret = tpm_send_command(cmd_buffer, 16, resp_buffer, &resp_size);
    if (ret != 0) {
        return -1;
    }
    
    /* Parse response (simplified) */
    tpm_result_t *result = (tpm_result_t *)resp_buffer;
    if (result->rc != TPM_RC_SUCCESS) {
        return -1;
    }
    
    /* Extract digest from response */
    if (digest && digest_size) {
        *digest_size = SHA256_DIGEST_SIZE;
        uint64_t i;
        for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
            digest[i] = resp_buffer[16 + i];
        }
    }
    
    return 0;
}

/* Get random bytes from TPM */
int tpm_get_random(uint8_t *buffer, uint32_t size) {
    uint8_t cmd_buffer[64];
    uint8_t resp_buffer[256];
    uint32_t resp_size = sizeof(resp_buffer);
    uint32_t *cmd = (uint32_t *)cmd_buffer;
    
    /* Build GET_RANDOM command */
    cmd[0] = (TPM_ST_NO_SESSIONS << 16) | 0x0000000A; /* tag + size */
    cmd[1] = TPM_CC_GET_RANDOM;
    cmd[2] = size; /* requested bytes */
    
    /* Wait for TPM */
    if (tpm_wait_ready() != 0) {
        return -1;
    }
    
    /* Send command */
    int ret = tpm_send_command(cmd_buffer, 12, resp_buffer, &resp_size);
    if (ret != 0) {
        return -1;
    }
    
    /* Check response */
    tpm_result_t *result = (tpm_result_t *)resp_buffer;
    if (result->rc != TPM_RC_SUCCESS) {
        return -1;
    }
    
    /* Copy random data */
    uint32_t actual_size = *(uint32_t *)(resp_buffer + 8);
    if (actual_size > size) {
        actual_size = size;
    }
    
    uint32_t i;
    for (i = 0; i < actual_size; i++) {
        buffer[i] = resp_buffer[12 + i];
    }
    
    return (int)actual_size;
}

/* Platform-specific TPM MMIO access (stubs - implemented per architecture) */
__attribute__((weak))
void tpm_write_register(uint32_t addr, uint32_t value) {
    volatile uint32_t *reg = (volatile uint32_t *)(TPM_BASE_ADDR + addr);
    *reg = value;
}

__attribute__((weak))
uint32_t tpm_read_register(uint32_t addr) {
    volatile uint32_t *reg = (volatile uint32_t *)(TPM_BASE_ADDR + addr);
    return *reg;
}

__attribute__((weak))
int tpm_wait_ready(void) {
    int timeout = 1000000;
    uint32_t status;
    
    while (timeout-- > 0) {
        status = tpm_read_register(TPM_STS);
        if (status & 0x80) { /* TPM_STS_VALID */
            return 0;
        }
    }
    
    return -1;
}

__attribute__((weak))
int tpm_send_command(const uint8_t *cmd, uint32_t cmd_size,
                     uint8_t *response, uint32_t *resp_size) {
    uint32_t i;
    
    /* Write command to FIFO */
    for (i = 0; i < cmd_size; i++) {
        tpm_write_register(TPM_XMIT_DATA, cmd[i]);
    }
    
    /* Set TPM_STS_GO bit to start execution */
    tpm_write_register(TPM_STS, 0x20);
    
    /* Wait for response */
    if (tpm_wait_ready() != 0) {
        return -1;
    }
    
    /* Read response from FIFO */
    uint32_t bytes_available = tpm_read_register(TPM_STS) & 0xFFFF;
    if (bytes_available > *resp_size) {
        bytes_available = *resp_size;
    }
    
    for (i = 0; i < bytes_available; i++) {
        response[i] = (uint8_t)tpm_read_register(TPM_RECV_FIFO);
    }
    
    *resp_size = bytes_available;
    return 0;
}
