#include <cJSON.h>
#include <crypto.h>
#include <curl/curl.h>
#include <netease_api.h>
#include <request.h>
#include <stdio.h>

int main(void) {
	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);

	char *response;
	user_followeds("cookies.txt", "582535183", 0, 100, "true", &response);
	printf("Response1:\n%s\n", response);
	free(response);
	curl_global_cleanup();

	return 0;
}
