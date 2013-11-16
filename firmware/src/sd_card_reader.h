/*
 * sd_card_reader.h
 *
 *  Created on: Mar 8, 2013
 *  Author: trol
 */

#ifndef _SD_CARD_READER_H_
#define _SD_CARD_READER_H_

#include <string.h>

bool sdcard_raw_inited = false;
struct partition_struct* sdcard_partition = 0;
struct fat_fs_struct* sdcard_fs = 0;
struct fat_dir_struct* sdcard_dd = 0;
struct fat_file_struct* sdcard_fd = 0;


volatile bool task_readNextBlock = false;


uint8_t sdcard_buf_1[512];
uint8_t sdcard_buf_2[512];
uint16_t sdcard_pos_read;
uint8_t sdcard_block;	// 0, 1
uint8_t sdcard_ready_f;
offset_t sdcard_offset;
char sdcard_currentDirectory[32];
struct fat_dir_entry_struct sdcard_directory;

cluster_t fat_get_next_cluster(const struct fat_fs_struct* fs, cluster_t cluster_num);



static uint8_t find_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name, struct fat_dir_entry_struct* dir_entry) {
    while ( fat_read_dir(dd, dir_entry) ) {
        if ( strcmp(dir_entry->long_name, name) == 0 )  {
            fat_reset_dir(dd);
            return 1;
        }
    }
    return 0;
}

static struct fat_file_struct* open_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name) {
    struct fat_dir_entry_struct file_entry;
    if ( !find_file_in_dir(fs, dd, name, &file_entry) ) {
        return 0;
    }
    return fat_open_file(fs, &file_entry);
}


/**
 * Inits SD-card device, opens partition and file system
 */
static inline bool sdrdr_init() {
	// setup sd card slot
	MSG("sdcard_raw_init");
	sdcard_raw_inited = sd_raw_init();
	if ( !sdcard_raw_inited ) {
		MSG("MMC/SD initialization failed");
		return false;
	}
    // open first partition
	MSG("partition_open");
	sdcard_partition = partition_open(sd_raw_read, sd_raw_read_interval,
#if SD_RAW_WRITE_SUPPORT
			sd_raw_write, sd_raw_write_interval,
#else
			0, 0,
#endif
			0);
	if ( !sdcard_partition ) {
        // If the partition did not open, assume the storage device is a "superfloppy", i.e. has no MBR.
		MSG("partition_open");
		sdcard_partition = partition_open(sd_raw_read,
				sd_raw_read_interval,
#if SD_RAW_WRITE_SUPPORT
				sd_raw_write, sd_raw_write_interval,
#else
				0, 0,
#endif
				-1);
	}
	if ( !sdcard_partition ) {
		MSG("opening partition failed");
		return false;
	}
	MSG("fat_open");
	sdcard_fs = fat_open(sdcard_partition);
	if ( !sdcard_fs ) {
		MSG("opening filesystem failed");
		return false;
	}

	// open root directory
	MSG("fat_get_dir_entry_of_path");
	sdcard_currentDirectory[0] = '/';
	sdcard_currentDirectory[1] = 0;
	if (!fat_get_dir_entry_of_path(sdcard_fs, sdcard_currentDirectory, &sdcard_directory) ) {
		sdcard_currentDirectory[0] = '/';
		sdcard_currentDirectory[1] = 0;
		fat_get_dir_entry_of_path(sdcard_fs, sdcard_currentDirectory, &sdcard_directory);
	}

/*
	MSG("fat_open_dir");
	sdcard_dd = fat_open_dir(sdcard_fs, &directory);
	if ( !sdcard_dd ) {
		MSG("opening root directory failed");
		return false;
	}


	// print directory listing
	struct fat_dir_entry_struct dir_entry;
	while ( fat_read_dir(dd, &dir_entry) ) {
		uint8_t spaces = sizeof(dir_entry.long_name) - strlen(dir_entry.long_name) + 4;

		uart_puts(dir_entry.long_name);
		uart_putc(dir_entry.attributes & FAT_ATTRIB_DIR ? '/' : ' ');
		while(spaces--)
			uart_putc(' ');
		uart_putdw_dec(dir_entry.file_size);
		uart_putc('\n');
	}

	struct fat_file_struct* fd = open_file_in_dir(sdcard_fs, dd, "settings");
	if ( !fd ) {
		uart_puts_p(PSTR("error opening "));
		uart_putc('\n');
		return false;
	}
	uint8_t buffer[8];
	uint32_t offset = 0;
	intptr_t count;
	while((count = fat_read_file(fd, buffer, sizeof(buffer))) > 0) {
		uart_putdw_hex(offset);
		uart_putc(':');
		for(intptr_t i = 0; i < count; ++i) {
			uart_putc(' ');
			uart_putc_hex(buffer[i]);
		}
		uart_putc('\n');
		offset += 8;
	}

	fat_close_file(fd);

	fat_close_dir(dd);
*/

	MSG("OK!");
	return true;
}

static bool sdrdr_openFile() {
	sdcard_fd = open_file_in_dir(sdcard_fs, sdcard_dd, "kino.snd");
	if ( !sdcard_fd ) {
		uart_puts_p(PSTR("error opening "));
		uart_putc('\n');
		return false;
	}
	uart_puts_p(PSTR("sound file is opened"));

	sdcard_pos_read = 0;
	sdcard_ready_f = 0;
	sdcard_block = 0;
	sdcard_offset = 0;

	bt_Reset(true);

	task_readNextBlock = true;
	return true;
}



static void sdrdr_closeFile() {
	if ( sdcard_fd ) {
		fat_close_file(sdcard_fd);
		sdcard_fd = 0;
	}
	task_readNextBlock = false;
	bt_Reset(false);
}




static inline void sdrdr_close() {
	if ( !sdcard_raw_inited ) {
		return;
	}
	if ( sdcard_dd ) {
		fat_close_dir(sdcard_dd);
		sdcard_dd = 0;
	}
	if ( sdcard_fs ) {
		fat_close(sdcard_fs);
		sdcard_fs = 0;
	}
	if ( sdcard_partition ) {
		partition_close(sdcard_partition);
		sdcard_partition = 0;
	}
}




static uint8_t sdrdr_nextByte() {
	uint8_t result;
	if ( sdcard_block == 0 && (sdcard_ready_f & _BV(0))) {
//		MSG_DEC("RET-1 ", sdcard_pos_read);
		result = sdcard_buf_1[sdcard_pos_read++];
		if ( sdcard_pos_read >= 512 ) {
			sdcard_pos_read = 0;
			sdcard_block = 1;
			sdcard_ready_f &= ~_BV(0);
//			MSG("E1");
		}
	} else if ( sdcard_block == 1 && (sdcard_ready_f & _BV(1)) ) {
//		MSG_DEC("RET-2 ", sdcard_pos_read);
		result = sdcard_buf_2[sdcard_pos_read++];
		if ( sdcard_pos_read >= 512 ) {
			sdcard_pos_read = 0;
			sdcard_block = 0;
			sdcard_ready_f &= ~_BV(1);
//			MSG("E2");
		}
	} else {
//		MSG("RET-X! ");
		return 0x80;
	}
	return result;

/*
	if ( rb_read_size() == 0 ) {
		return 0x80;
	}
	return rb_pop();
*/
/*
	uint8_t result =  sdcard_buffer[sdcard_offset++];
	if ( sdcard_offset >= sizeof(sdcard_buffer) ) {
		intptr_t cnt = fat_read_file(sdcard_fd, sdcard_buffer, sizeof(sdcard_buffer));
		sdcard_offset = 0;
		if ( cnt == 0 ) {
			fat_seek_file(sdcard_fd, 0, FAT_SEEK_SET);
//			audio_disable();
//			sdrdr_closeFile();
//			sdrdr_close();
//			bt_Reset(false);	// TODO !!!
		}
	}
	return result;
*/
}

static void sdrdr_readNext() {
	intptr_t cnt;
	if ( !(sdcard_ready_f & _BV(0)) ) {
		cnt = fat_read_file(sdcard_fd, sdcard_buf_1, sizeof(sdcard_buf_1));
		sdcard_offset += cnt;
		sdcard_ready_f |= _BV(0);
		if ( cnt < sizeof(sdcard_buf_1) ) {
			sdrdr_closeFile();
			sdrdr_openFile();
			//sdrdr_close();
		}
	}
	if ( !(sdcard_ready_f & _BV(1)) ) {
		cnt = fat_read_file(sdcard_fd, sdcard_buf_2, sizeof(sdcard_buf_2));
		sdcard_offset += cnt;
		sdcard_ready_f |= _BV(1);
		if ( cnt < sizeof(sdcard_buf_2) ) {
			sdrdr_closeFile();
			sdrdr_openFile();
			//sdrdr_close();
		}
	}
}

#endif // _SD_CARD_READER_H_
