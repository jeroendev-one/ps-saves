#include <stdint.h>
#include <stddef.h>
#include <zip.h>

#define countof(_array) (sizeof(_array) / sizeof(_array[0]))
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define SUCCESS 0
#define FAILURE -1

extern uint32_t user_id;
extern char hex_userid[9];

void notify_popup(const char *p_Uri, const char *p_Format, ...);
void klog(const char* FMT, ...);
void dump_data(const u8 *data, u64 size);
int initialize_jbc(void);
int patch_save_libraries(void);
int file_exists(const char *path);
int dir_exists(const char *path);
int unlink_secure(const char *path);
int read_buffer(const char *file_path, uint8_t **buf, size_t *size);

int zip_directory(const char* basedir, const char* inputdir, const char* output_filename);
void walk_zip_directory(const char* startdir, const char* inputdir, struct zip *zipper);
void set_file_compression(struct zip *archive, zip_uint64_t index, int compression_level);

