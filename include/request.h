#ifdef __cplusplus
extern "C" {
#endif
typedef enum { NCM_WEAPI, NCM_EAPI, NCM_LINUXAPI, C_API_COUNT } API_TYPE;
#include <cJSON.h>
int NCM_request(const char *cookieJar, cJSON *paramJSON, cJSON *customCookies, const char *url,const char* szEapiURL, char **outResponse,
				API_TYPE NCMAPIType);
#ifdef __cplusplus
}
#endif