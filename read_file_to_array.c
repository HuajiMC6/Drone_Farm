#include "drivers.h"

int read_file_to_array(const char* filename, uint8_t* buffer, uint32_t max_size) {
	FIL file;
	FRESULT res;
    UINT bytes_read;
    
    res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
        f_mount(NULL, "", 0);
        return -1;
    }
    
    FSIZE_t file_size = f_size(&file);
    if (file_size > max_size) {
        f_close(&file);
        f_mount(NULL, "", 0);
        return -1;
    }
    
    res = f_read(&file, buffer, file_size, &bytes_read);
    if (res != FR_OK || bytes_read != file_size) {
        f_close(&file);
        f_mount(NULL, "", 0);
        return -1;
    }
    
    f_close(&file);
    
    return bytes_read;
}
