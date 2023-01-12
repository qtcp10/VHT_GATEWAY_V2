/*
 * SPIFFS_user.c
 *
 *  Created on: Sep 28, 2021
 *      Author: HAOHV6
 */
#include "SPIFFS_user.h"

/* Get total message count value in NVS
   Return an error if anything goes wrong
   during this process.
 */
static const char *TAG = "FFS";

esp_err_t writetofile( char* FileName,char* textbuffer)
{
	ESP_LOGI(TAG, "Opening file");
	char *base_path = "/spiffs";
	char file[64];
	sprintf(file, "%s/%s", base_path, FileName);
	FILE* f = NULL;
	f = fopen(file, "w");
	if (f == NULL) {
		ESP_LOGE(TAG, "Failed to open file for writing");
		return ESP_FAIL;
	}

	char* tp = textbuffer;
	while(*tp != '\0') {
		fputc(*tp++,f);
	}
	fclose(f);
	ESP_LOGI(TAG, "File written");
	return ESP_OK;
}

esp_err_t readfromfile(char* FileName,char* textbuffer)
{
	char *base_path = "/spiffs";
	char file[64];
	sprintf(file, "%s/%s", base_path, FileName);
    FILE* f = NULL;
    ESP_LOGI(TAG, "Reading file");
    f = fopen(file, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[1024];
	fgets(line, sizeof(line), f);
	fclose(f);
	// strip newline
	char *pos = strchr(line, '\n');
	if (pos) {
		*pos = '\0';
	}
	strcpy(textbuffer, line);
	ESP_LOGI(TAG, "Read from file: '%s'", textbuffer);
	return ESP_OK;
}

void mountSPIFFS()
{
	ESP_LOGI(TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t conf = {
	  .base_path = "/spiffs",
	  .partition_label = NULL,
	  .max_files = 5,
	  .format_if_mount_failed = true
	};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
	}
}

void removeFile(char * Filename){
	struct stat st;
	if (stat(Filename, &st) == 0) {
		// Delete it if it exists
		unlink(Filename);
	}
}
