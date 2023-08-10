#ifdef __cplusplus
extern "C" {
#endif

int captcha_sent(const char *cookieJar, const char *phone, const char *ctcode, char **response);
int user_follows(const char *cookieJar, const char *uid, const char *offset, const char *limit, const char *order,
				 char **response);
int user_followeds(const char *cookieJar, const char *uid, int offset,  int limit, const char *order,
				   char **response);
int captcha_verify(const char *cookieJar, const char *cellphone, const char *ctcode, const char *captcha, char **response);
int login_qr_key(const char *cookieJar, char **response);
int login_qr_check(const char *cookieJar, const char *key, char **response);
int logout(const char *cookieJar, char **response);
#ifdef __cplusplus
}
#endif