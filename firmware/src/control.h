/*
 * control.h
 *
 *  Created on: Feb 24, 2013
 *      Author: trol
 */

#ifndef _CONTROL_H_
#define _CONTROL_H_

#define CMD_UPDATE				1	// update firmware
#define CMD_SET_TIME			2	// set time
#define CMD_GET_TIME			3	// get time
#define CMD_SET_BRIGHTNESS		4	// set brightness
#define CMD_DISK_INFO			5	// get info about memory card
#define CMD_GET_LIGHT			6	// get value from light sensor
#define CMD_GET_SOUND			7	// get value from sound sensor


#define CMD_CD					8
#define CMD_LS					9
#define CMD_FREAD				10
#define CMD_FWRITE				11
#define CMD_MKDIR				12
#define CMD_RM					13
#define CMD_FTIME				14
#define CMD_FSIZE				15

#define CMD_FPLAY				16


#define CMD_HELP				17


#define CMD_MMC					18
#define CMD_TEST				19


#define CMD_NOT_FOUND		0	// unknown command


PROGMEM const uint8_t control_commands[] = {
	CMD_UPDATE, 			'U', 'P', 'D', 'A', 'T', 'E', 0,
	CMD_SET_TIME,			'S', 'E', 'T' ,'_', 'T', 'I', 'M', 'E', 0,
	CMD_GET_TIME,			'G', 'E', 'T' ,'_', 'T', 'I', 'M', 'E', 0,
	CMD_SET_BRIGHTNESS,		'S', 'E', 'T' ,'_', 'B', 'R', 'I', 'G', 'H', 'T', 'N', 'E', 'S', 'S', 0,
	CMD_DISK_INFO,			'D', 'I', 'S', 'K',  '_', 'I', 'N', 'F', 'O', 0,
	CMD_GET_LIGHT,			'G', 'E', 'T', '_', 'L', 'I', 'G', 'H', 'T', 0,
	CMD_GET_SOUND,			'G', 'E', 'T', '_', 'S', 'O', 'U', 'N', 'D', 0,

	CMD_CD,					'C', 'D', 0,
	CMD_LS,					'L', 'S', 0,
	CMD_FREAD,				'F', 'R', 'E', 'A', 'D', 0,
	CMD_FWRITE,				'F', 'W', 'R', 'I', 'T', 'E', 0,
	CMD_MKDIR,				'M', 'K', 'D', 'I', 'R', 0,
	CMD_RM,					'R', 'M', 0,
	CMD_FTIME,				'F', 'T', 'I', 'M', 'E', 0,
	CMD_FSIZE,				'F', 'S', 'I', 'Z', 'E', 0,
	CMD_FPLAY,				'F', 'P', 'L', 'A', 'Y', 0,

	CMD_HELP,				'H', 'E', 'L', 'P', 0,

	
	CMD_MMC,				'M', 'M', 'C', 0,
	CMD_TEST,				'T', 'E', 'S', 'T', 0,

	0
};



typedef void (*ptr_t)(void) __attribute__ ((noreturn));


static inline void jumpPtr(uint16_t address) {
	ptr_t ptr = (ptr_t)address;
	ptr();
}



/**
 * str     pointer to source string buffer
 * strArgs here will be stored pointer to end of command. if the command contains space here will be stored pointer to first symbol after space. else here will be stored pointer to 0
 */
static inline uint8_t getCommandCode(char *str, char **strArgs) {
	uint16_t pos = 0;

	while ( pos <  1024 ) {	// TODO use sizeof !!!
		uint8_t cmdCode = pgm_read_byte(&control_commands[pos++]);
		if ( cmdCode == 0 ) {
			return CMD_NOT_FOUND;
		}
		bool equals = true;
		for ( uint8_t i = 0; i < 0xff; i++ ) {	// TODO !!!
			uint8_t ch = pgm_read_byte(&control_commands[pos++]);
			if ( ch == 0 ) {
				*strArgs = str + i;
				break;
			}
			if ( ch == ' ' ) {
				*strArgs = str + i;
				break;
			}
			if ( str[i] != ch ) {
				equals = false;
			}
		}
		if ( equals ) {
			return cmdCode;
		}
	}
	return CMD_NOT_FOUND;
}


/**
 *
 */
static char* skipSpaces(char *str) {
	for ( uint8_t i = 0; i < 0xff; i++ ) {	// TODO !!!
		if ( str[i] != ' ' ) {
			return str + i;
		}
	}
	return str;	// TODO
}

/**
 *
 */
static char* parseInt(char *str, uint16_t *value) {
	*value = 0;
	char *result = str;
	for (  ; ; ) {
		char ch = result[0];
		if ( ch >= '0' && ch <= '9' ) {
			*value = *value * 10 + ch - '0';
		} else {
			return result;
		}
		result++;
	}
}


static inline bool cmdSetTime(char *str) {
	uint16_t val;
	uint8_t h, m, s;

	char *strArgs = skipSpaces(str);
	strArgs = parseInt(strArgs, &val);
	h = val;
	if ( strArgs[0] != ':' ) {
		return false;
	}
	strArgs = parseInt(strArgs+1, &val);
	m = val;
	if ( strArgs[0] != ':' ) {
		return false;
	}
	strArgs = parseInt(strArgs+1, &val);
	s = val;

	if ( h > 23 || m > 59 || s > 59 ) {
		return false;
	}
	if ( !pcf8583_setTime(h, m, s, 0) ) {

	}
	time_hour = h;
	time_min = m;
	time_sec = s;
	onMinuteChanged();
	return true;
}

bool diskInit() {
	if (!sdcard_raw_inited) {
		sdcard_raw_inited = sdrdr_init();
	}
	if (!sdcard_raw_inited) {
		uart_puts_p(PSTR("MMC/SD card init error\n")); 
	}
	return sdcard_raw_inited;
}


static inline void cmdDiskInfo() {
	struct sd_raw_info disk_info;

	if ( !diskInit() ) {
		return;
	}
	if ( !sd_raw_get_info(&disk_info) ) {
		uart_puts_p(PSTR("can't get info"));
		return;
	}
	offset_t fsSize = fat_get_fs_size(sdcard_fs);
	offset_t fsFree = fat_get_fs_free(sdcard_fs);
	uart_puts_p(PSTR("Manufacturer: 0x")); uart_putc_hex(disk_info.manufacturer); uart_putc('\n');
	uart_puts_p(PSTR("OEM: ")); uart_puts((char*) disk_info.oem); uart_putc('\n');
	uart_puts_p(PSTR("Product: ")); uart_puts((char*) disk_info.product); uart_putc('\n');
	uart_puts_p(PSTR("Revision: ")); uart_putc_hex(disk_info.revision); uart_putc('\n');
	uart_puts_p(PSTR("Serial: 0x")); uart_putdw_hex(disk_info.serial); uart_putc('\n');
	uart_puts_p(PSTR("Date: ")); uart_putw_dec(disk_info.manufacturing_month); uart_putc('/'); uart_putw_dec(disk_info.manufacturing_year); uart_putc('\n');
    uart_puts_p(PSTR("Size: ")); uart_putdw_dec(disk_info.capacity / 1024 / 1024); uart_puts_p(PSTR("MB\n"));
    uart_puts_p(PSTR("Copy: ")); uart_putw_dec(disk_info.flag_copy); uart_putc('\n');
    uart_puts_p(PSTR("Write protect: ")); uart_putw_dec(disk_info.flag_write_protect_temp); uart_putc('/'); uart_putw_dec(disk_info.flag_write_protect); uart_putc('\n');
    uart_puts_p(PSTR("Format: ")); uart_putw_dec(disk_info.format); uart_putc('\n');
    uart_puts_p(PSTR("Size: ")); uart_putdw_dec(fsSize); uart_putc('\n');
    uart_puts_p(PSTR("Free: ")); uart_putdw_dec(fsFree); uart_putc('\n');

	//sdrdr_close();
}

static inline bool cmdSetBrightness(char *str) {
	uint16_t val;

	char *strArgs = skipSpaces(str);
	strArgs = parseInt(strArgs, &val);
	display_autoBrightness = val > BRIGHTNESS_MAX;
	if ( val <= BRIGHTNESS_MAX) {
		display_setBrightness(val);
	}
	return true;
}



static inline void cmdDiskCd(char *path) {
	if (!diskInit()) {
		return;
	}

	struct fat_dir_entry_struct subdir_entry;
	if (find_file_in_dir(sdcard_fs, sdcard_dd, path, &subdir_entry)) {
		struct fat_dir_struct* dd_new = fat_open_dir(sdcard_fs, &subdir_entry);
		if (dd_new) {
			uart_puts_p(PSTR("OK\n"));
			return;
		}
	}

	uart_puts_p(PSTR("directory not found\n"));
}


static bool openDirectory() {
	MSG("fat_open_dir"); 	MSG_STR(sdcard_currentDirectory);
	sdcard_dd = fat_open_dir(sdcard_fs, &sdcard_directory);
	if ( !sdcard_dd ) {
		uart_puts_p(PSTR("can't open dir\n"));
		return false;
	}
	return true;
}

static inline void cmdDiskLs() {
	if (!diskInit()) {
		return;
	}
	if (!openDirectory()) {
		return;
	}

	// print directory listing
	struct fat_dir_entry_struct dir_entry;
	while ( fat_read_dir(sdcard_dd, &dir_entry) ) {
		uint8_t spaces = sizeof(dir_entry.long_name) - strlen(dir_entry.long_name) + 4;

		uart_puts(dir_entry.long_name);
		uart_putc(dir_entry.attributes & FAT_ATTRIB_DIR ? '/' : ' ');
		while (spaces--) {
			uart_putc(' ');
		}
		uart_putdw_dec(dir_entry.file_size);
		uart_putc('\n');
	}
	fat_close_dir(sdcard_dd);
	sdcard_dd = 0;
	//sdrdr_close();
}

static inline void cmdDiskFread(char *path) {
	if (!diskInit()) {
		return;
	}
	//sdrdr_close();
}

static inline void cmdDiskFwrite(char *path) {
	if (!diskInit()) {
		return;
	}
	//sdrdr_close();
}

static inline void cmdDiskMkdir(char *path) {
	if (!diskInit()) {
		return;
	}
	if (!openDirectory()) {
		return;
	}

	struct fat_dir_entry_struct dir_entry;
	if ( fat_create_dir(sdcard_dd, path, &dir_entry) ) {
		uart_puts_p(PSTR("OK\n")); 
	} else {
		uart_puts_p(PSTR("can't create dir\n")); 
	}
	fat_close_dir(sdcard_dd);
	sdcard_dd = 0;
}


static inline void cmdDiskRm(char *path) {
	if (!diskInit()) {
		return;
	}
	if (!openDirectory()) {
		return;
	}
	struct fat_dir_entry_struct dir_entry;

	if ( find_file_in_dir(sdcard_fs, sdcard_dd, path, &dir_entry) ) {
		if ( fat_delete_file(sdcard_fs, &dir_entry) ) {
			uart_puts_p(PSTR("OK\n"));
			return;
		}
	}

	fat_close_dir(sdcard_dd);
	sdcard_dd = 0;
}

static inline void cmdDiskPrintFtime(char *path) {
	if (!diskInit()) {
		return;
	}
	//sdrdr_close();
}

static inline void cmdDiskPrintFsize(char *path) {
	if (!diskInit()) {
		return;
	}
	//sdrdr_close();
}

static inline void cmdDiskPlayFile(char *path) {
	if (!diskInit()) {
		return;
	}
	//sdrdr_close();
}

static inline void cmdHelp() {

}



static inline void onCommand(char *str) {
	char *strArgs;
	uint8_t cmd = getCommandCode(str, &strArgs);


//	MSG_DEC("=?===> ", cmd);

	switch ( cmd ) {
		case CMD_UPDATE:
			sdrdr_close();
			cli();
			jumpPtr(0x7E00);
			return;

		case CMD_SET_TIME:
			if ( !cmdSetTime(strArgs) ) {
				uart_puts_p(PSTR("usage: SET_TIME hh:mm:ss\n"));
			} else {
				uart_puts_p(PSTR("OK"));
			}
			return;

		case CMD_GET_TIME:
			uart_putw_dec(time_hour);
			uart_putc(':');
			uart_putw_dec(time_min);
			uart_putc(':');
			uart_putw_dec(time_sec);
			return;

		case CMD_SET_BRIGHTNESS:
			cmdSetBrightness(strArgs);
			return;

		case CMD_DISK_INFO:
			cmdDiskInfo();
			return;

		case CMD_GET_LIGHT:
			uart_putw_dec(lightValue);
			break;

		case CMD_GET_SOUND:
			uart_putw_dec(soundValue);
			break;

		case CMD_CD:
			cmdDiskCd(strArgs);
			break;

		case CMD_LS:
			cmdDiskLs();
			break;

		case CMD_FREAD:
			cmdDiskFread(strArgs);
			break;

		case CMD_FWRITE:
			cmdDiskFwrite(strArgs);
			break;

		case CMD_MKDIR:
			cmdDiskMkdir(strArgs);
			break;

		case CMD_RM:
			cmdDiskRm(strArgs);
			break;

		case CMD_FTIME:
			cmdDiskPrintFtime(strArgs);
			break;

		case CMD_FSIZE:
			cmdDiskPrintFsize(strArgs);
			break;

		case CMD_FPLAY:
			cmdDiskPlayFile(strArgs);
			break;

		case CMD_HELP:
sdrdr_close();
			cmdHelp();
			break;



		case CMD_MMC:
//			bt_Reset(true);

//			return; 	// !!!!!!!!!!!!!!!!!!!!!

			if ( !sdrdr_init() ) {
				MSG("MMC ERROR!");
				return;
			} else {
				MSG("MMC OK!!!");
			}
//			sdrdr_close();


//			sdrdr_init();
			if ( sdrdr_openFile() ) {
				MSG("OPEN - OK");
				audio_enable();
//				bt_Reset(true);
			} else {
				MSG("OPEN - FAIL");
			}
			onMinuteChanged();
			break;

		case CMD_TEST:
			MSG_DEC("LIGHT = ", lightValue);
			MSG_DEC("BR = ", calculateLight2brightness(lightValue));
			break;

		default:
			uart_puts_p(PSTR("invalid command"));
			uart_puts(str);
			break;
	}

}


#endif // _CONTROL_H_
