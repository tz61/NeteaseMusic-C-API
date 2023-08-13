#ifdef __cplusplus
extern "C" {
#endif
typedef enum { NCM_PLAYLIST_NORMAL, NCM_PLAYLIST_VIDEO, NCM_PLAYLIST_SHARED } PLAYLIST_TYPE;
int captcha_sent(const char *cookieJar, const char *phone, const char *ctcode, char **response);
int user_follows(const char *cookieJar, const char *uid, const char *offset, const char *limit, const char *order,
				 char **response);
int user_followeds(const char *cookieJar, const char *uid, int offset, int limit, const char *order, char **response);
int captcha_verify(const char *cookieJar, const char *cellphone, const char *ctcode, const char *captcha, char **response);
int login_qr_key(const char *cookieJar, char **response);
int login_qr_check(const char *cookieJar, const char *key, char **response);
int logout(const char *cookieJar, char **response);
int playlist_create(const char *cookieJar, const char *name, int isPrivacy, PLAYLIST_TYPE playlistType, char **response);
int login_refresh(const char *cookieJar, char **response);
int personalized(const char *cookieJar, char **response);
int personalized_newsong(const char *cookieJar, char **response);
int like(const char *cookieJar, const char *trackId, int isLiked, char **response);
int homepage_block_page(const char *cookieJar, int isRefresh, const char *cursor, char **response);
int user_account(const char *cookieJar, char **response);
int user_level(const char *cookieJar, char **response);
int user_binding(const char *cookieJar, const char *uid, char **response);
int simi_user(const char *cookieJar, const char *songId, const char *limit, const char *offset, char **response);
int recommend_resource(const char *cookieJar, char **response);
int recommend_songs(const char *cookieJar, char **response);
int history_recommend_songs(const char *cookieJar, char **response);
int history_recommend_songs_detail(const char *cookieJar, const char *date, char **response);
int vip_info(const char *cookieJar, char **response);
int record_recent_song(const char *cookieJar, char **response);
int song_url_v1(const char *cookieJar, const char *ids, const char *level, char **response);
#ifdef __cplusplus
}
#endif