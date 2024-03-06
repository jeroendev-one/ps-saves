#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <orbis/Sysmodule.h>
#include <orbis/SaveData.h>
#include <time.h>
#include <unistd.h>
#include <sqlite3.h>

#include <zip.h>
#include <dirent.h>

#include "config_ini.h"
#include "saves.h"
#include "util.h"
#include "http.h"
#include "orbisPad.h"

#define SAVES_CONFIG "/data/saves/saves.ini"

uint32_t user_id;
char save_db_path[64];
char hex_userid[9];

#define BUFFER_SIZE 4096 // Should be enough

int initModules(){
    if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE) < 0)
        return FAILURE;

    if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE) < 0)
        return FAILURE;

    if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SAVE_DATA) < 0)
        return FAILURE;

    if(sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET) < 0)
        return FAILURE;

    if(sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP) < 0)
        return FAILURE;

    if(sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL) < 0)
        return FAILURE;

    if(sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NETCTL) < 0)
        return FAILURE;

    return SUCCESS;
}

static int initPad(void)
{
	if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_PAD) < 0)
		return 0;

	// Initialize the Pad library
	if (orbisPadInit() < 0)
	{
		klog("[ERROR] FAILURE to initialize pad library!");
		return 0;
	}

	// Get the user ID
	user_id = orbisPadGetConf()->userId;

    // Convert the user ID to hexadecimal
    snprintf(hex_userid, sizeof(hex_userid), "%08x", user_id);

    // Print the hexadecimal user ID
    klog("Hexadecimal User ID: %s\n", hex_userid);
	

	return 1;
}

static void terminate(void)
{
	klog("Exiting...");

	// Unload loaded libraries
	if (unpatch_SceShellCore())
		notify_popup("cxml://psnotification/tex_default_icon_notification", "PS4 Save patches removed from memory");

	terminate_jbc();
    sleep(5);
	sceSystemServiceLoadExec("exit", NULL);
}

static sqlite3* open_sqlite_db(const char* db_path)
{
	uint8_t* db_buf;
	size_t db_size;
	sqlite3 *db;

	if (sqlite3_memvfs_init("orbis_rw") != SQLITE_OK_LOAD_PERMANENTLY)
	{
		klog("Error loading extension: %s", "memvfs");
		return NULL;
	}

	if (read_buffer(db_path, &db_buf, &db_size) != SUCCESS)
	{
		klog("Cannot open database file: %s", db_path);
		return NULL;
	}

	// And open that memory with memvfs now that it holds a valid database
	char *memuri = sqlite3_mprintf("file:memdb?ptr=0x%p&sz=%lld&freeonclose=1", db_buf, db_size);
	klog("Opening '%s' (%ld bytes)...", db_path, db_size);

	if (sqlite3_open_v2(memuri, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_URI, "memvfs") != SQLITE_OK)
	{
		klog("Error open memvfs: %s", sqlite3_errmsg(db));
		return NULL;
	}
	sqlite3_free(memuri);

	if (sqlite3_exec(db, "PRAGMA journal_mode = OFF;", NULL, NULL, NULL) != SQLITE_OK)
		klog("Error set pragma: %s", sqlite3_errmsg(db));

	return db;
}

int get_save_entries(const char *db_path, save_entry_t **save_entries) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    const char *sql = "SELECT title_id, dir_name, blocks FROM savedata WHERE title_id like '%CUSA%'";

    if (sqlite3_open(db_path, &db) != SQLITE_OK) {
        klog("-1 to open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        klog("-1 to prepare SQL statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    int index = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *title_id = (const char *)sqlite3_column_text(stmt, 0);
        const char *dir_name = (const char *)sqlite3_column_text(stmt, 1);
        int blocks = sqlite3_column_int(stmt, 2);

        // Resize the save_entries array
        *save_entries = realloc(*save_entries, (index + 1) * sizeof(save_entry_t));

        // Populate the save_entry_t struct like mother earth
        (*save_entries)[index].title_id = strdup(title_id);
        (*save_entries)[index].dir_name = strdup(dir_name);
        (*save_entries)[index].blocks = blocks;

        index++;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // Return index.html
    return index;
}




int main(void) {
    // Init required modules
    initModules();

    http_init();

    // Init OrbisPad
    initPad();

   	// Jailbreak
	initialize_jbc();

    // Apply save patches
    patch_save_libraries();
    
    sleep(5);

    snprintf(save_db_path, sizeof(save_db_path), "/system_data/savedata/%s/db/user/savedata.db", hex_userid);
    save_entry_t *save_entries = NULL;
    int num_entries = get_save_entries(save_db_path, &save_entries);

    // Load INI config
    IniData iniData = {0};
    parse_ini_file(SAVES_CONFIG, &iniData);

    klog("global.backup_method: %s\n", get_value(&iniData, "global", "backup_method"));
    klog("ftp.server: %s\n", get_value(&iniData, "ftp", "server"));

    // With limit of 20
    for (int i = 0; i < num_entries && i < 20; i++) {
        
        // Log how far we are
        klog("Processing entry %d out of %d\n", i + 1, num_entries);

        // Construct the save path based on the user ID and title ID
        char save_path[128];
        snprintf(save_path, sizeof(save_path), "/user/home/%s/savedata/%s", hex_userid, save_entries[i].title_id);

        // Define the save entry
        save_entry_t save_entry = {
            .title_id = strdup(save_entries[i].title_id),
            .path = save_path,
            .dir_name = strdup(save_entries[i].dir_name),
            .blocks = save_entries[i].blocks,
            .flags = 1024,
            .type = 0,
            .codes = NULL
        };

        // Mount the save in read-only mode
        char mount_path[ORBIS_SAVE_DATA_MOUNT_POINT_DATA_MAXSIZE];
        
        int result = orbis_SaveMount(&save_entry, ORBIS_SAVE_DATA_MOUNT_MODE_RDONLY, mount_path);
        if (result != 1) {
            klog("-1 to mount save: Title ID: %s, Directory Name: %s\n", save_entry.title_id, save_entry.dir_name);
            continue;  // Maybe next run you will be lucky
        }

        klog("Mounted save: Title ID: %s, Directory Name: %s\n", save_entry.title_id, save_entry.dir_name);
        
        // Zip part
        char output_filename[128];
        snprintf(output_filename, sizeof(output_filename), "/mnt/usb0/exports/%s_%s.zip", save_entry.title_id, save_entry.dir_name);

        // Generate file_name based on save_entry
        char file_name[128];
        snprintf(file_name, sizeof(file_name), "%s_%s.zip", save_entry.title_id, save_entry.dir_name);

        // Generate file_path based on output_filename
        char file_path[128];
        snprintf(file_path, sizeof(file_path), "%s/%s_%s.zip", "/mnt/usb0/exports", save_entry.title_id, save_entry.dir_name);

        // ZIP directories
        const char *basedir = "/mnt/sandbox/SVBD00004_000/";
        const char *inputdir = "/mnt/sandbox/SVBD00004_000/savedata0/";

        int zip_result = zip_directory(basedir, inputdir, output_filename);
        if (zip_result) {
            klog("Directory '%s' zipped to '%s' successfully.\n", inputdir, output_filename);
        } else {
            klog("Failed to zip directory '%s' to '%s'.\n", inputdir, output_filename);
        }

        const char *ftp_server = get_value(&iniData, "ftp", "server");
        const char *username = get_value(&iniData, "ftp", "username");
        const char *password = get_value(&iniData, "ftp", "password");
        const char *ftp_port_str = get_value(&iniData, "ftp", "port");
        int ftp_port = atoi(ftp_port_str);

        // Upload the file via FTP
        int ftp_result = upload_via_ftp(ftp_server, file_path, file_name, ftp_port, username, password);
        if (ftp_result) {
            klog("File uploaded successfully.\n");
        } else {
            klog("Failed to upload file.\n");
        }

        klog("Backup done. Title ID: %s, Save: %s \n", save_entry.title_id, save_entry.dir_name);

        // Unmount the save
        int umount_result = orbis_SaveUmount(mount_path);
        if (umount_result) {
            klog("Unmounted save: Title ID: %s, Directory Name: %s\n", save_entry.title_id, save_entry.dir_name);
        } else {
            klog("-1 to unmount save: Title ID: %s, Directory Name: %s\n", save_entry.title_id, save_entry.dir_name);
        }

    }


    //const char* url = "https://jeroendev.one/upload/";
    //const char* local_file = "/data/GoldHEN/plugins.ini";
    //int result = http_get(url);

    //http_get(url);
    //http_post(url);
    //http_send(url, local_file);

    http_end();

    // Shutdown
    terminate();

    for (;;) {}
    return 0;
}
