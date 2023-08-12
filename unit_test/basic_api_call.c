/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/
/* <DESC>
 * simple HTTP POST using the easy interface
 * </DESC>
 */
#include <cJSON.h>
#include <crypto.h>
#include <curl/curl.h>
#include <netease_api.h>
#include <request.h>
#include <stdio.h>
#include <synchapi.h>

int main(void) {
	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);
	char phone[50], captcha[10];
	char *response;
	const char *cookieJar = "testjar.txt";
// user_follows(cookieJar, "582535183", "0", "1", "true", &response);
// printf("Response1:\n%s\n", response);
// free(response);

// user_followeds(cookieJar, "582535183", 0, 1, "true", &response);
// printf("Response2:\n%s\n", response);
// free(response);

// printf("Please enter phone number to get captcha\n");
// scanf_s("%s",phone,sizeof(phone));
// int code = captcha_sent(cookieJar, phone, "86", &response);
// printf("Response3:\n%s\n", response);
// free(response);
// printf("code=%d\n", code);
#if 0
	char szUnikey[100] = {0};
	login_qr_key(cookieJar, &response);
	printf("Response3:\n%s\n", response);
	cJSON *resJson = cJSON_Parse(response);
	cJSON *unikey = cJSON_GetObjectItemCaseSensitive(resJson, "unikey");
	cJSON *code;
	int nCode;

	free(response);
	if (cJSON_IsString(unikey) && unikey->valuestring) {
		strcpy_s(szUnikey, 100, unikey->valuestring);
		printf("please click https://music.163.com/login?codekey=%s\n \n", unikey->valuestring);
	}
	cJSON_Delete(resJson);

	while (1) {
		login_qr_check(cookieJar, szUnikey, &response);

		cJSON *resJson = cJSON_Parse(response);
		code = cJSON_GetObjectItemCaseSensitive(resJson, "code");
		if (cJSON_IsNumber(code)) {
			nCode = code->valueint;
		}
		cJSON_Delete(code);
		free(response);
		switch (nCode) {
		case 801:
			printf("Wait for scanning...\n");
			break;
		case 802:
			printf("Already scanned, please press login button...\n");
			break;
		case 803:
			printf("Login successfull!\n");
			break;
		}
		if (nCode == 803)
			break;

		Sleep(1000);
	}

	// logout(cookieJar, &response);
	// printf("Response5:\n%s\n", response);
	// free(response);
#else
	// personalized_newsong(cookieJar, &response);
	// like 22686719
	//playlist_create(cookieJar, "Touhou Gameset", 1, NCM_PLAYLIST_NORMAL, &response);
	//like(cookieJar, "22686719", 1, &response);
	homepage_block_page(cookieJar, 1, 0, &response);
	printf("Response5:\n%s\n", response);
	free(response);
#endif
	curl_global_cleanup();
	return 0;
}
