

#define HTTP_SUCCESS    1
#define HTTP_FAILED     0
#define HTTP_USER_AGENT "Mozilla/5.0 (PLAYSTATION 4; 1.00)"

#define BUFFER_SIZE 4096
#define API_KEY "apitestkey"

int http_post(const char* url);
int http_get(const char* url);
int http_send(const char* url, const char* local_file);
int upload_via_ftp(const char *ftp_server, const char *file_path, const char *file_name, int ftp_port, const char *username, const char *password);
void http_end(void);