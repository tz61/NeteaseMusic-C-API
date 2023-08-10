#ifdef __cplusplus
extern "C" {
#endif
#include <cJSON.h>
int NCM_weapi(cJSON *object, char **dataParam);
unsigned char *base64(unsigned char *input, int inputLength, size_t *outputLength);
int NCM_eapi(cJSON *object, const char *pszUrl, char **dataParam);
#ifdef __cplusplus
}
#endif