#include "http.h"

#include <curl/curl.h>
#include <orbis/Sysmodule.h>
#include <orbis/libkernel.h>
#include <orbis/Net.h>
#include <orbis/NetCtl.h>
#include "util.h"
#include <string.h>
#include <stdlib.h>

// Struct to hold response data and buffer size
struct ResponseData {
    char buffer[BUFFER_SIZE];
    size_t size;
};

// Define a callback function to handle the response data
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    struct ResponseData *response_data = (struct ResponseData *)userdata;

    // Calculate the total size of the data
    size_t total_size = size * nmemb;

    // Check if the buffer can accommodate the new data
    if (response_data->size + total_size >= BUFFER_SIZE) {
        // Buffer overflow, handle error
        return 0;
    }

    // Copy the received data to the buffer
    memcpy(response_data->buffer + response_data->size, ptr, total_size);
    response_data->size += total_size;

    // Return the total size of the data
    return total_size;
}

int http_init() {
    if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
        return HTTP_FAILED;

    // Return HTTP_SUCCESS if init succeeds
    return HTTP_SUCCESS;
}

int http_post(const char* url) {
    CURL *curl;
    CURLcode res;

    // Initialize response data structure
    struct ResponseData response_data = {0};

    curl = curl_easy_init();
    if (!curl) {
        klog("Error: cURL init failed.");
        return HTTP_FAILED;
    }

    klog("POST URL: %s \n", url);

    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "key=apitestkey");

    // Temporarily till I figure this fucking shit out
    struct curl_slist *resolve_list = NULL;
    resolve_list = curl_slist_append(resolve_list, "jeroendev.one:443:51.38.55.115");
    curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve_list);

    // not sure how to use this when enabled
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    // not sure how to use this when enabled
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    // Set SSL VERSION to TLS 1.2
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    // Set user agent string
    curl_easy_setopt(curl, CURLOPT_USERAGENT, HTTP_USER_AGENT);
    // Set timeout for the connection to build
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    // Fail the request if the HTTP code returned is equal to or larger than 400
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    // Set the callback function and the response data structure as the userdata
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    // Perform the request
    res = curl_easy_perform(curl);

    long httpresponsecode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpresponsecode);

    char response_str[512];
    snprintf(response_str, sizeof(response_str), "HTTP Response Code: %ld \n", httpresponsecode);
    klog(response_str);

    // Cleanup
    curl_easy_cleanup(curl);

    // Cleanup temp list
    curl_slist_free_all(resolve_list);

    if (res == CURLE_OK) {
        klog("Response: %s", response_data.buffer);
    } else {
        klog("curl_easy_perform() failed: %s", curl_easy_strerror(res));
        notify_popup("cxml://psnotification/tex_icon_ban", "HTTP init failed");
    }

    return HTTP_SUCCESS;
}

int http_get(const char* url)
{
    //char full_url[1024];
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (!curl)
    {
        klog("ERROR: CURL INIT");
        return HTTP_FAILED;
    }

    klog("URL: %s \n", url);

    // Temporarily till I figure this fucking shit out
    struct curl_slist *resolve_list = NULL;
    resolve_list = curl_slist_append(resolve_list, "jeroendev.one:443:51.38.55.115");
    curl_easy_setopt(curl, CURLOPT_RESOLVE, resolve_list);
    
    // not sure how to use this when enabled
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	// not sure how to use this when enabled
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	// Set SSL VERSION to TLS 1.2
	curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    // Set user agent string
    curl_easy_setopt(curl, CURLOPT_USERAGENT, HTTP_USER_AGENT);
    // Set SSL VERSION to TLS 1.2
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    // Set timeout for the connection to build
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    // Follow redirects (?)
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    // maximum number of redirects allowed
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 20L);
    // Fail the request if the HTTP code returned is equal to or larger than 400
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    // Enable verbose output
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    // Perform the request
    res = curl_easy_perform(curl);

    long httpresponsecode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpresponsecode);

    char response_str[128];
    snprintf(response_str, sizeof(response_str), "HTTP Response Code: %ld \n", httpresponsecode);
    klog(response_str);

    // Cleanup
    curl_easy_cleanup(curl);

    // Cleanup temp list
    curl_slist_free_all(resolve_list);

    if (res != CURLE_OK)
    {
        klog("curl_easy_perform() failed: %s", curl_easy_strerror(res));
        return HTTP_FAILED;
    }

    return HTTP_SUCCESS;
}

int http_send(const char* url, const char* local_file)
{
    CURL *curl;
    CURLcode res;

    // Initialize libcurl
    curl = curl_easy_init();
    if (!curl) {
        klog("ERROR: Failed to initialize libcurl\n");
        return 1;
    }

    // Open the local file for reading
    FILE *file = fopen(local_file, "rb");
    if (!file) {
        klog("ERROR: Failed to open file: %s\n", local_file);
        curl_easy_cleanup(curl);
        return 1;
    }

    // Determine size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for file content
    char *filedata = (char *)malloc(filesize + 1);
    if (!filedata) {
        klog("ERROR: Failed to allocate memory for file content \n");
        fclose(file);
        curl_easy_cleanup(curl);
        return 1;
    }

    // Read the content of the file into memory
    size_t readsize = fread(filedata, 1, filesize, file);
    fclose(file);

    // Terminate the file data with a null terminator
    filedata[readsize] = '\0';

    // Set response data callback
    struct ResponseData response_data = {0};

    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url);

    // Set POST data
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, filedata);


    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    // not sure how to use this when enabled
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	// not sure how to use this when enabled
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	// Set SSL VERSION to TLS 1.2
	curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    // Set user agent string
    curl_easy_setopt(curl, CURLOPT_USERAGENT, HTTP_USER_AGENT);
    // Set SSL VERSION to TLS 1.2
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

    // Perform the request
    res = curl_easy_perform(curl);

    // Print the received data
    if (res == CURLE_OK) {
        klog("Response: %s\n", response_data.buffer);
    } else {
        klog("ERROR: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    // Clean up
    curl_easy_cleanup(curl);
    free(filedata);

    return (res == CURLE_OK) ? 0 : 1;
}


long long filesize(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return -1; // Error opening file
    }

    fseek(file, 0, SEEK_END);
    long long size = ftell(file);

    fclose(file);

    return size;
}

int upload_via_ftp(const char *ftp_server, const char *file_path, const char *file_name, int ftp_port, const char *username, const char *password) {
    CURL *curl;
    CURLcode res = CURLE_OK;
    int success = 0;

    // Initialize libcurl
    curl = curl_easy_init();
    if (curl) {
        // Set FTP server URL
        char url[256];
        snprintf(url, sizeof(url), "ftp://%s:%d/%s", ftp_server, ftp_port, file_name);
        klog("FTP URL: %s\n", url);
        curl_easy_setopt(curl, CURLOPT_URL, url);

        // Set FTP username and password
        char userpwd[128];
        snprintf(userpwd, sizeof(userpwd), "%s:%s", username, password);
        klog("Credentials: %s", userpwd);
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);

        // Set option to create missing directories
        curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);

        // Set file to upload
        FILE *file = fopen(file_path, "rb");
        if (file == NULL) {
            perror("Error opening file");
            curl_easy_cleanup(curl);
            return success;
        }

        fseek(file, 0, SEEK_END);
        curl_off_t file_size = ftell(file);
        fseek(file, 0, SEEK_SET); // Rewind file pointer

        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READDATA, file);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, file_size);

        // Perform the FTP upload
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            klog("Upload successful.\n");
            success = 1;
        } else {
            klog("Failed to upload: %s\n", curl_easy_strerror(res));
        }

        // Clean up libcurl
        fclose(file);
        curl_easy_cleanup(curl);
    } else {
        klog("Failed to initialize libcurl.\n");
    }

    return success;
}



void http_end(void) {
    // Cleanup and shutdown HTTP related stuff
    curl_global_cleanup();
    sceNetCtlTerm();
    sceSysmoduleUnloadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NETCTL);
    sceSysmoduleUnloadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET);
}
