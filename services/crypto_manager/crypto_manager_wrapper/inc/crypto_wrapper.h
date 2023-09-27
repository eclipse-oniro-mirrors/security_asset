#include "hks_api.h"

#ifdef __cplusplus
extern "C" {
#endif

int EncryptWrapper(uint32_t keyLen, const uint8_t *keyData, uint32_t aadLen, const uint8_t *aad,
    uint32_t msgLen, const uint8_t *msg, uint32_t cipherLen, uint8_t *cipher);
int DecryptWrapper(uint32_t keyLen, const uint8_t *keyData, uint32_t aadLen, const uint8_t *aad,
    uint32_t cipherLen, const uint8_t *cipher, uint32_t plainLen, uint8_t *plain);

#ifdef __cplusplus
}
#endif