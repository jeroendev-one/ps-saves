#include <time.h>
#include <stdint.h>

#define MAX_USB_DEVICES         8
#define USB0_PATH               "/mnt/usb0/"
#define USB1_PATH               "/mnt/usb1/"

// Save flags
#define SAVE_FLAG_LOCKED        1
#define SAVE_FLAG_OWNER         2
#define SAVE_FLAG_SELECTED      4
#define SAVE_FLAG_ZIP           8
#define SAVE_FLAG_PS2           16
#define SAVE_FLAG_PSP           32
#define SAVE_FLAG_PSV           64
#define SAVE_FLAG_TROPHY        128
#define SAVE_FLAG_ONLINE        256
#define SAVE_FLAG_PS4           512
#define SAVE_FLAG_HDD           1024

typedef struct option_entry
{
    char * line;
    char * * name;
    char * * value;
    int id;
    int size;
    int sel;
} option_entry_t;

typedef struct code_entry
{
    uint8_t type;
    uint8_t flags;
    char * name;
    char * file;
    uint8_t activated;
    int options_count;
    char * codes;
    option_entry_t * options;
} code_entry_t;


typedef struct list_node_s
{
	void *value;
	struct list_node_s *next;
} list_node_t;

typedef struct list_s
{
	list_node_t *head;
	size_t count;
} list_t;

typedef struct save_entry
{
    char * name;
	char * title_id;
	char * path;
	char * dir_name;
    uint32_t blocks;
	uint16_t flags;
    uint16_t type;
    list_t * codes;
} save_entry_t;

typedef struct
{
    list_t * list;
    char path[128];
    char* title;
    uint8_t icon_id;
    void (*UpdatePath)(char *);
    int (*ReadCodes)(save_entry_t *);
    list_t* (*ReadList)(const char*);
} save_list_t;

list_t * ReadUsbList(const char* userPath);
list_t * ReadUserList(const char* userPath);
list_t * ReadOnlineList(const char* urlPath);
list_t * ReadBackupList(const char* userPath);
list_t * ReadTrophyList(const char* userPath);
void UnloadGameList(list_t * list);
char * readTextFile(const char * path, long* size);
int sortSaveList_Compare(const void* A, const void* B);
int sortSaveList_Compare_TitleID(const void* A, const void* B);
int sortCodeList_Compare(const void* A, const void* B);
int ReadCodes(save_entry_t * save);
int ReadTrophies(save_entry_t * game);
int ReadOnlineSaves(save_entry_t * game);
int ReadBackupCodes(save_entry_t * bup);

int http_init(void);
void http_end(void);
int http_download(const char* url, const char* filename, const char* local_dst, int show_progress);

int extract_7zip(const char* zip_file, const char* dest_path);
int extract_rar(const char* rar_file, const char* dest_path);
int extract_zip(const char* zip_file, const char* dest_path);


int show_dialog(int dialog_type, const char * format, ...);
int osk_dialog_get_text(const char* title, char* text, uint32_t size);
void init_progress_bar(const char* msg);
void update_progress_bar(uint64_t progress, const uint64_t total_size, const char* msg);
void end_progress_bar(void);
#define show_message(...)	show_dialog(DIALOG_TYPE_OK, __VA_ARGS__)

int init_loading_screen(const char* message);
void stop_loading_screen(void);
void disable_unpatch();

void execCodeCommand(code_entry_t* code, const char* codecmd);

int appdb_rebuild(const char* db_path, uint32_t userid);
int appdb_fix_delete(const char* db_path, uint32_t userid);
int addcont_dlc_rebuild(const char* db_path);

int regMgr_GetParentalPasscode(char* passcode);
int regMgr_GetUserName(int userNumber, char* outString);
int regMgr_GetAccountId(int userNumber, uint64_t* psnAccountId);
int regMgr_SetAccountId(int userNumber, uint64_t* psnAccountId);

int create_savegame_folder(const char* folder);
int get_save_details(const save_entry_t *save, char** details);
int orbis_SaveUmount(const char* mountPath);
int orbis_SaveMount(const save_entry_t *save, uint32_t mode, char* mountPath);
int orbis_UpdateSaveParams(const char* mountPath, const char* title, const char* subtitle, const char* details, uint32_t up);

