#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <orbis/libkernel.h>
#include <zip.h>

void notify_popup(const char *p_Uri, const char *p_Format, ...)
{
    OrbisNotificationRequest s_Request;
    memset(&s_Request, '\0', sizeof(s_Request));

    s_Request.reqId = NotificationRequest;
    s_Request.unk3 = 0;
    s_Request.useIconImageUri = 0;
    s_Request.targetId = -1;

    // Maximum size to move is destination size - 1 to allow for null terminator
    if (p_Uri)
    {
        s_Request.useIconImageUri = 1;
        strlcpy(s_Request.iconUri, p_Uri, sizeof(s_Request.iconUri));
    }

    va_list p_Args;
    va_start(p_Args, p_Format);
    vsnprintf(s_Request.message, sizeof(s_Request.message), p_Format, p_Args);
    va_end(p_Args);

    sceKernelSendNotificationRequest(NotificationRequest, &s_Request, sizeof(s_Request), 0);
}

void klog(const char* FMT, ...) {
    char MessageBuf[1024];
    va_list args;
    va_start(args, FMT);
    vsprintf(MessageBuf, FMT, args);
    va_end(args);

    sceKernelDebugOutText(0, MessageBuf);
}

void dump_data(const u8 *data, u64 size) {
	u64 i;
	for (i = 0; i < size; i++)
		klog("%02X", data[i]);
}

int read_buffer(const char *file_path, uint8_t **buf, size_t *size) {
        FILE *fp;
        uint8_t *file_buf;
        size_t file_size;

        if ((fp = fopen(file_path, "rb")) == NULL)
                return -1;
        fseek(fp, 0, SEEK_END);
        file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        file_buf = (uint8_t *)malloc(file_size);
        fread(file_buf, 1, file_size, fp);
        fclose(fp);

        if (buf)
                *buf = file_buf;
        else
                free(file_buf);
        if (size)
                *size = file_size;

        return 0;
}

int file_exists(const char *path)
{
    if (access(path, F_OK) == 0) {
    	return SUCCESS;
    }
    
	return -1;
}

int dir_exists(const char *path)
{
    struct stat sb;
    if ((stat(path, &sb) == 0) && S_ISDIR(sb.st_mode)) {
        return SUCCESS;
    }
    return -1;
}

int unlink_secure(const char *path)
{   
    if(file_exists(path)==SUCCESS)
    {
        chmod(path, 0777);
		return remove(path);
    }
    return -1;
}

void set_file_compression(struct zip *archive, zip_uint64_t index, int compression_level) {
    if (zip_set_file_compression(archive, index, ZIP_CM_DEFLATE, compression_level) < 0) {
        klog("-1 to set file compression level for index %llu\n", (unsigned long long)index);
    }
}

void walk_zip_directory(const char* startdir, const char* inputdir, struct zip *zipper)
{
    char fullname[256]; 
    struct dirent *dirp;
    int len = strlen(startdir) + 1;
    DIR *dp = opendir(inputdir);

    if (!dp) {
        klog("-1 to open input directory: '%s'\n", inputdir);
        return;
    }

    if (strlen(inputdir) > len)
    {
        //klog("Adding folder '%s'\n", inputdir+len);
        if (zip_add_dir(zipper, inputdir+len) < 0)
        {
            klog("-1 to add directory to zip: %s\n", inputdir);
            return;
        }
    }

    while ((dirp = readdir(dp)) != NULL) {
        if ((strcmp(dirp->d_name, ".")  != 0) && (strcmp(dirp->d_name, "..") != 0)) {
            snprintf(fullname, sizeof(fullname), "%s%s", inputdir, dirp->d_name);

            if (dirp->d_type == DT_DIR) {
                strcat(fullname, "/");
                walk_zip_directory(startdir, fullname, zipper);
            } else {
                struct zip_source *source = zip_source_file(zipper, fullname, 0, 0);
                if (!source) {
                    klog("-1 to source file to zip: %s\n", fullname);
                    continue;
                }
                //klog("Adding file '%s'\n", fullname+len);
                zip_int64_t zidx = zip_add(zipper, &fullname[len], source);
                if (zidx < 0) {
                    zip_source_free(source);
                    klog("-1 to add file to zip: %s\n", fullname);
                    continue;
                }
                set_file_compression(zipper, zidx, 9); // Setting compression level to 9 (highest)
                zip_file_set_external_attributes(zipper, zidx, 0, ZIP_OPSYS_UNIX, (zip_uint32_t)(0100644) << 16);
            }
        }
    }
    closedir(dp);
}

int zip_directory(const char* basedir, const char* inputdir, const char* output_filename)
{
    struct zip *archive = zip_open(output_filename, ZIP_CREATE | ZIP_TRUNCATE, NULL);

    if (!archive) {
        klog("-1 to open output file '%s'\n", output_filename);
        return 0;
    }

    klog("Zipping <%s> to %s...\n", inputdir, output_filename);
    walk_zip_directory(basedir, inputdir, archive);
    zip_close(archive);

    return (file_exists(output_filename) == SUCCESS);
}