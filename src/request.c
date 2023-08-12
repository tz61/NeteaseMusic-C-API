#include <cJSON.h>
#include <crypto.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define REQUEST_DEBUG 0

typedef struct {
	char *responseBuffer;
	size_t size;
} RESPONSE;
typedef enum { NCM_WEAPI, NCM_EAPI, NCM_LINUXAPI, C_API_COUNT } API_TYPE;

static size_t post_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	RESPONSE *pResponse = (RESPONSE *)userp;
	char *ptr = realloc(pResponse->responseBuffer, pResponse->size + realsize + 1);
	if (!ptr) {
		printf("[post_write_callback] out of memory\n");
		free(pResponse->responseBuffer);
		pResponse->responseBuffer = NULL;
		return 0;
	}
	pResponse->responseBuffer = ptr;
	memcpy(pResponse->responseBuffer + pResponse->size, contents, realsize);
	pResponse->size += realsize;
	pResponse->responseBuffer[pResponse->size] = 0;
	return realsize;
}

/** NCM_request
 * @param cookieJar filename to store the cookie and to read from
 * @return -1 while weapi failed, otherwise 0 when successful
 * need to free cJSON parameter `paramJSON` manually after invokation of this function
 */
// TODO to randomly pick user agent
int NCM_request(const char *cookieJar, cJSON *paramJSON, cJSON *customCookies, const char *url, const char *szEapiURL,
				char **outResponse, API_TYPE NCMAPIType) {

	CURL *curlHandle;
	CURLcode result;
	RESPONSE response = {.responseBuffer = malloc(0), .size = 0};
	char *postFields;
	char szCustomCookie[4000] = {0};

	// Set relevant headers

	curlHandle = curl_easy_init();
	if (curlHandle) {
		/* First set the URL that is about to receive our POST. This URL can
		   just as well be an https:// URL if that is what should receive the
		   data. */
		struct curl_slist *headerList = NULL;
		headerList = curl_slist_append(
			headerList, "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:80.0) Gecko/20100101 Firefox/80.0");
		headerList = curl_slist_append(headerList, "Content-Type: application/x-www-form-urlencoded");
		if (strstr(url, "music.163.com"))
			headerList = curl_slist_append(headerList, "Referer: https://music.163.com");
		headerList = curl_slist_append(headerList, "X-Real-IP: ::1");
		headerList = curl_slist_append(headerList, "X-Forwarded-For: ::1");
		// set request url
		curl_easy_setopt(curlHandle, CURLOPT_URL, url);

		// set request type
		curl_easy_setopt(curlHandle, CURLOPT_HTTPPOST, 1);

		// set ssl verify and certificates
		curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYSTATUS, 0);
		curl_easy_setopt(curlHandle, CURLOPT_CAINFO, "cacert-2023-05-30.pem");
		curl_easy_setopt(curlHandle, CURLOPT_CAPATH, "cacert-2023-05-30.pem");

		// set write callback and response buffer to save to
		curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, post_write_callback);
		curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, (void *)&response);

		// set cookie file to read from
		curl_easy_setopt(curlHandle, CURLOPT_COOKIEFILE, cookieJar);
		// set cookiejar file to save to
		curl_easy_setopt(curlHandle, CURLOPT_COOKIEJAR, cookieJar);

		// check existing cookies to find csrf_token and then add it into data param
		// force a reload for checking
		curl_easy_setopt(curlHandle, CURLOPT_COOKIELIST, "RELOAD");

		// Add custom cookies
		cJSON *_tmp;
		if (customCookies) {
			_tmp = customCookies->child;
			while (_tmp) {
				if (_tmp->type == cJSON_String) {
					char szTmp[1000];
					// MUSIC_A/U and csrf_token is ultra long!
					sprintf_s(szTmp, 1000, "%s=%s; ", _tmp->string, _tmp->valuestring);
					strcat_s(szCustomCookie, 2000, szTmp);
				}
				_tmp = _tmp->next;
			}
		}

		struct curl_slist *existingCookies;
		curl_easy_getinfo(curlHandle, CURLINFO_COOKIELIST, &existingCookies);
		struct curl_slist *each = existingCookies;
		unsigned char csrfTokenFlag = 0, bMUSIC_U = 0, bMUSIC_A = 0;
		char szSavedCsrfToken[100] = {0}, szSavedMUSIC_A[500] = {0}, szSavedMUSIC_U[2000] = {0};
		while (each) {
			char *tmpLocation;
			if ((tmpLocation = strstr(each->data, "__csrf"))) {
#if REQUEST_DEBUG
				printf("__csrf=%s\n", tmpLocation + 7); // csrf_token\t
#endif
				strcpy_s(szSavedCsrfToken, 100, tmpLocation + 11);
				csrfTokenFlag = 1;
			}
			if ((tmpLocation = strstr(each->data, "MUSIC_A"))) {
				strcpy_s(szSavedMUSIC_A, 500, tmpLocation + 8);
				bMUSIC_A = 1;
			}
			if ((tmpLocation = strstr(each->data, "MUSIC_U"))) {
#if REQUEST_DEBUG
				printf("MUSIC_U=%s\n", tmpLocation + 8); // MUSIC_U\t
#endif
				strcpy_s(szSavedMUSIC_U, 2000, tmpLocation + 8);
				bMUSIC_U = 1;
			}
			each = each->next;
		}
		curl_slist_free_all(existingCookies);
		if (!bMUSIC_A && !bMUSIC_U) {
			bMUSIC_A = 1;
			strcpy_s(szSavedMUSIC_A, 500,
					 "bf8bfeabb1aa84f9c8c3906c04a04fb864322804c83f5d607e91a04eae463c9436bd1a17ec353cf780b396507a3f7464e8a60f4bbc0"
					 "19437993166e004087dd32d1490298caf655c2353e58daa0bc13cc7d5c198250968580b12c1b8817e3f5c807e650dd04abd3fb8130b"
					 "7ae43fcc5b");
		}
		switch (NCMAPIType) {
		case NCM_WEAPI:

			if (!csrfTokenFlag) {
#if REQUEST_DEBUG
				printf("csrf_token=(null)\n");
#endif
				cJSON_AddStringToObject(paramJSON, "csrf_token", "");
			} else {
				cJSON_AddStringToObject(paramJSON, "csrf_token", szSavedCsrfToken);
			}
			// encrypt post fields
			if (NCM_weapi(paramJSON, &postFields)) {
				printf("error in weapi\n");

				curl_easy_cleanup(curlHandle);
				curl_slist_free_all(headerList);
				return -1;
			}
			break;
		case NCM_EAPI:
			cJSON *eapiCustomCookie = cJSON_CreateObject();

			// note we need also add these eapiCustomCookie to post fields
			cJSON_AddStringToObject(eapiCustomCookie, "osver", "11");
			// 	osver: cookie.osver, //系统版本
			size_t nbase64size;
			char *pszBase64Str =
				base64((unsigned char *)"869687054311451\t02:00:00:00:00:00\t5106025eb79a5247\t70ffbaac7", 60, &nbase64size);
			cJSON_AddStringToObject(eapiCustomCookie, "deviceId", pszBase64Str);

			free(pszBase64Str);
			//  deviceId: cookie.deviceId, //encrypt.base64.encode(imei + '\t02:00:00:00:00:00\t5106025eb79a5247\t70ffbaac7')
			cJSON_AddStringToObject(eapiCustomCookie, "appver", "8.9.70");
			// appver: cookie.appver || '8.9.70', // app版本
			cJSON_AddStringToObject(eapiCustomCookie, "versioncode", "140");
			// versioncode: cookie.versioncode || '140', //版本号
			cJSON_AddStringToObject(eapiCustomCookie, "mobilename", "Pixel 5");
			// mobilename: cookie.mobilename, //设备model

			// compile time
			cJSON_AddStringToObject(eapiCustomCookie, "buildver", "PD2046B_A_5.16.1");
			// buildver: cookie.buildver || Date.now().toString().substr(0, 10),
			cJSON_AddStringToObject(eapiCustomCookie, "resolution", "1920x1080");
			// resolution: cookie.resolution || '1920x1080', //设备分辨率
			cJSON_AddStringToObject(eapiCustomCookie, "__csrf", szSavedCsrfToken);
			// __csrf: csrfToken,
			cJSON_AddStringToObject(eapiCustomCookie, "os", "android");
			// os: cookie.os || 'android',

			// dirty fix
			// cJSON_AddStringToObject(eapiCustomCookie, "channel", "undefined");
			// channel: cookie.channel,
			char szRequestID[50] = {0};
			sprintf_s(szRequestID, 50, "%d_%04d", (int)time(0), rand() % 1000);
			cJSON_AddStringToObject(eapiCustomCookie, "requestId", szRequestID);
			// requestId: `${Date.now()}_${Math.floor(Math.random() * 1000)
			//   .toString()
			//   .padStart(4, '0')}`,
			if (bMUSIC_U) {
				cJSON_AddStringToObject(eapiCustomCookie, "MUSIC_U", szSavedMUSIC_U);
			} else if (bMUSIC_A) {
				cJSON_AddStringToObject(eapiCustomCookie, "MUSIC_A", szSavedMUSIC_A);
			}
			cJSON_AddItemToObject(paramJSON, "header", eapiCustomCookie);

			// now calc the final post fields
			if (NCM_eapi(paramJSON, szEapiURL, &postFields)) {
				printf("error in weapi\n");

				curl_easy_cleanup(curlHandle);
				curl_slist_free_all(headerList);
				return -1;
			}
			// Then copy the header to real headers
			cJSON *tmp = eapiCustomCookie->child;
			while (tmp) {
				if (tmp->type == cJSON_String) {
					char szTmp[1000];
					// MUSIC_A/U and csrf_token is ultra long!
					sprintf_s(szTmp, 1000, "%s=%s; ", tmp->string, tmp->valuestring);
					strcat_s(szCustomCookie, 2000, szTmp);
				}
				tmp = tmp->next;
			}

			break;
		}
#if REQUEST_DEBUG
		printf("Overall Added cookie:\n%s\n", szCustomCookie);
#endif
		curl_easy_setopt(curlHandle, CURLOPT_COOKIE, szCustomCookie);
		// set request header
		curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, headerList);

		// set post fields
		curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, postFields);
		// printf("postfields:\n%s\n", postParam);
		/* Perform the request, res will get the return code */

		result = curl_easy_perform(curlHandle);
		/* Check for errors */
		if (result != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
#if REQUEST_DEBUG
		// get response cookies
		struct curl_slist *cookies = NULL;
		result = curl_easy_getinfo(curlHandle, CURLINFO_COOKIELIST, &cookies);
		if (!result && cookies) {
			/* a linked list of cookies in cookie file format */
			struct curl_slist *each = cookies;
			while (each) {
				printf("%s\n", each->data);
				each = each->next;
			}
			/* we must free these cookies when we are done */
			curl_slist_free_all(cookies);

			struct curl_header *header;
			if (CURLHE_OK == curl_easy_header(curlHandle, "Content-Type", 0, CURLH_HEADER, -1, &header))
				printf("Got content-type: %s\n", header->value);

			printf("All server headers:\n");
			{
				struct curl_header *h;
				struct curl_header *prev = NULL;
				do {
					h = curl_easy_nextheader(curlHandle, CURLH_HEADER, -1, prev);
					if (h)
						printf(" %s: %s (%u)\n", h->name, h->value, (int)h->amount);
					prev = h;
				} while (h);
			}
		}
#endif
		// free buffers
		free(postFields);
		/* always cleanup */
		curl_easy_cleanup(curlHandle);
		curl_slist_free_all(headerList);
	}
#if REQUEST_DEBUG
	printf("[request]response size:%d\n", response.size);
#endif
	if (NCMAPIType == NCM_EAPI) {
		// do decrypt
	}
	*outResponse = response.responseBuffer;
	// execute successfully
	return 0;
}