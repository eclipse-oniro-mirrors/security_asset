#include "hks_api.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t InitParamSet(struct HksParamSet **paramSet, const struct HksParam *params, uint32_t paramcount);
int32_t GenerateKey(uint32_t keyLen, const uint8_t *keyData);
int32_t DeleteKey(uint32_t keyLen, const uint8_t *keyData);
int32_t KeyExist(uint32_t keyLen, const uint8_t *keyData);

#ifdef __cplusplus
}
#endif