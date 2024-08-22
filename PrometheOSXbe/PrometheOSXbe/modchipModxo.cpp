#include "modchipModxo.h"
#include "crc32.h"
#include "settingsManager.h"

#define MODXO_LCD_DELAY 2

#define MODXO_REGISTER_LCD_DATA 0xDEA8
#define MODXO_REGISTER_LCD_COMMAND 0xDEA9
#define MODXO_REGISTER_BANKING 0xDEAA
#define MODXO_REGISTER_SIZE 0xDEAB
#define MODXO_REGISTER_MEM_ERASE 0xDEAC
#define MODXO_REGISTER_CHIP_ID 0xDEAD
#define MODXO_REGISTER_MEM_FLUSH 0xDEAE
#define MODXO_REGISTER_LED_COMMAND 0xA2
#define MODXO_REGISTER_LED_DATA 0xA3

#define MODXO_BANK_TSOP 0xc0
#define MODXO_BANK_MODXO 0x00 
#define MODXO_BANK_BOOTLOADER 0x01
#define MODXO_BANK_PROMETHEOS1 0x02
#define MODXO_BANK_PROMETHEOS2 0x03

#define MODXO_TD_INIT                   0x00
#define MODXO_TD_CLEAR_DISPLAY          0x10
#define MODXO_TD_RETURN_HOME            0x20
#define MODXO_TD_ENTRY_MODE_SET         0x30
#define MODXO_TD_DISPLAY_CONTROL        0x40
#define MODXO_TD_CURSOR_DISPLAY_SHIFT   0x50
#define MODXO_TD_SET_CURSOR_POSITION    0x60
#define MODXO_TD_SET_CONTRAST           0x70
#define MODXO_TD_SET_BACKLIGHT          0x71
#define MODXO_TD_SELECT_CUSTOM_CHAR     0xe0
#define MODXO_TD_SEND_CUSTOM_CHAR_DATA  0xf0

#define MODXO_LED_CMD_SET_LED_COUNT_STRIP 0x00
#define MODXO_LED_CMD_FILL_STRIP_COL 0x01
#define MODXO_LED_CMD_UPDATE_STRIPS 0x02
#define MODXO_LED_CMD_SET_PIX_IDX 0x03
#define MODXO_LED_CMD_WRITE_PIX_COL 0x04
#define MODXO_LED_CMD_SET_PIX_COUNT 0x05
#define MODXO_LED_CMD_WRITE_EFFECT 0x06
#define MODXO_LED_CMD_SELECT_COLOR 0xF0
#define MODXO_LED_CMD_SELECT_STRIP 0xFC

#define EFFECT_PIXELS_SHIFT_UP 0x01
#define EFFECT_PIXELS_SHIFT_DOWN 0x02
#define EFFECT_PIXELS_FADE 0x03
#define EFFECT_PIXELS_RANDOM 0x04
#define EFFECT_PIXELS_RAINBOW 0x05
#define EFFECT_PIXELS_BRIGHTNESS 0x06

//
//#define MODXO_BANK_SLOT1_256K 0x04
//#define MODXO_BANK_SLOT2_256K 0x05
//#define MODXO_BANK_SLOT3_256K 0x06
//#define MODXO_BANK_SLOT4_256K 0x07
//#define MODXO_BANK_SLOT5_256K 0x08
//#define MODXO_BANK_SLOT6_256K 0x09
//#define MODXO_BANK_SLOT7_256K 0x0a
//#define MODXO_BANK_SLOT8_256K 0x0b
//#define MODXO_BANK_SLOT9_256K 0x0c
//#define MODXO_BANK_SLOT10_256K 0x0d
//#define MODXO_BANK_SLOT11_256K 0x0e
//#define MODXO_BANK_SLOT12_256K 0x0f
//
//#define MODXO_BANK_PROM1_512K 0x40
//#define MODXO_BANK_PROM2_512K 0x41
//#define MODXO_BANK_SLOT1_512K 0x42
//#define MODXO_BANK_SLOT2_512K 0x43
//#define MODXO_BANK_SLOT3_512K 0x44
//#define MODXO_BANK_SLOT4_512K 0x45
//#define MODXO_BANK_SLOT5_512K 0x46
//#define MODXO_BANK_SLOT6_512K 0x47
//
//#define MODXO_BANK_PROM1_1024K 0x80
//#define MODXO_BANK_SLOT1_1024K 0x81
//#define MODXO_BANK_SLOT2_1024K 0x82
//#define MODXO_BANK_SLOT3_1024K 0x83
//#define MODXO_BANK_SLOT4_1024K 0x84
//#define MODXO_BANK_SLOT5_1024K 0x85
//#define MODXO_BANK_SLOT6_1024K 0x86
//#define MODXO_BANK_SLOT7_1024K 0x87
//#define MODXO_BANK_SLOT8_1024K 0x88
//#define MODXO_BANK_SLOT9_1024K 0x89
//#define MODXO_BANK_SLOT10_1024K 0x8a
//#define MODXO_BANK_SLOT11_1024K 0x8b
//#define MODXO_BANK_SLOT12_1024K 0x8c
//#define MODXO_BANK_SLOT13_1024K 0x8d
//#define MODXO_BANK_SLOT14_1024K 0x8e
//#define MODXO_BANK_SLOT15_1024K 0x8f

#define MODXO_SETTINGS_BANK MODXO_BANK_PROMETHEOS2
#define MODXO_SETTINGS_OFFSET (0x040000 - 0x1000)

//Bank Name      Bank Value        Offset    Size
//
//Tsop           1XXX XXXX         NA        NA        
//
//Modxo_256k     0000 0000         0x000000  0x040000 
//PromBL_256k    0000 0001         0x040000  0x040000
//PromOS1_256k   0000 0010         0x080000  0x040000
//PromOS2_256k   0000 0011         0x0c0000  0x040000
//
//Bank01_256k    0000 0100         0x100000  0x040000
//Bank02_256k    0000 0101         0x140000  0x040000
//Bank03_256k    0000 0110         0x180000  0x040000
//Bank04_256k    0000 0111         0x1c0000  0x040000
//
//Bank05_256k    0000 1000         0x200000  0x040000 (4+16mb only)
//Bank06_256k    0000 1001         0x240000  0x040000 (4+16mb only)
//Bank07_256k    0000 1010         0x280000  0x040000 (4+16mb only)
//Bank08_256k    0000 1011         0x2c0000  0x040000 (4+16mb only)
//
//Bank09_256k    0000 1100         0x300000  0x040000 (4+16mb only)
//Bank10_256k    0000 1101         0x340000  0x040000 (4+16mb only)
//Bank11_256k    0000 1110         0x380000  0x040000 (4+16mb only)
//Bank12_256k    0000 1111         0x3c0000  0x040000 (4+16mb only)
//
//Prom01_512k    0001 0000         0x000000  0x080000
//Prom02_512k    0001 0001         0x080000  0x080000
//Bank01_512k    0001 0010         0x100000  0x080000
//Bank02_512k    0001 0011         0x180000  0x080000
//Bank03_512k    0001 0100         0x200000  0x080000 (4+16mb only)
//Bank04_512k    0001 0101         0x280000  0x080000 (4+16mb only)
//Bank05_512k    0001 0110         0x300000  0x080000 (4+16mb only)
//Bank06_512k    0001 0111         0x380000  0x080000 (4+16mb only)
//
//Prom01_1024k   0010 0000         0x000000  0x100000
//Bank01_1024k   0010 0001         0x100000  0x100000
//Bank02_1024k   0010 0010         0x200000  0x100000 (4+16mb only)
//Bank03_1024k   0010 0011         0x300000  0x100000 (4+16mb only)
//Bank04_1024k   0010 0100         0x400000  0x100000 (16mb only)
//Bank05_1024k   0010 0101         0x500000  0x100000 (16mb only)
//Bank06_1024k   0010 0110         0x600000  0x100000 (16mb only)
//Bank07_1024k   0010 0111         0x700000  0x100000 (16mb only)
//Bank08_1024k   0010 1000         0x800000  0x100000 (16mb only)
//Bank09_1024k   0010 1001         0x900000  0x100000 (16mb only)
//Bank10_1024k   0010 1010         0xa00000  0x100000 (16mb only)
//Bank11_1024k   0010 1011         0xb00000  0x100000 (16mb only)
//Bank12_1024k   0010 1100         0xc00000  0x100000 (16mb only)
//Bank13_1024k   0010 1101         0xd00000  0x100000 (16mb only)
//Bank14_1024k   0010 1110         0xe00000  0x100000 (16mb only)
//Bank15_1024k   0010 1111         0xf00000  0x100000 (16mb only)

modchipModxo::modchipModxo()
{
	mBank = MODXO_BANK_BOOTLOADER;
}

void modchipModxo::setLedColor(uint8_t ledColor)
{
	do
	{
		Sleep(10);
	}
	while (inputByte(MODXO_REGISTER_LED_COMMAND) == 0);

	outputByte(MODXO_REGISTER_LED_COMMAND, MODXO_LED_CMD_SELECT_STRIP);
	outputByte(MODXO_REGISTER_LED_DATA, 0);

	outputByte(MODXO_REGISTER_LED_COMMAND, MODXO_LED_CMD_SET_PIX_COUNT);
	outputByte(MODXO_REGISTER_LED_DATA, 255); //This count is Pixel Count - 1, thus 255 is 256 pixels

	outputByte(MODXO_REGISTER_LED_COMMAND, MODXO_LED_CMD_FILL_STRIP_COL);
	outputByte(MODXO_REGISTER_LED_DATA, (ledColor & 1) == 1 ? 0xff : 0x00);
	outputByte(MODXO_REGISTER_LED_DATA, (ledColor & 2) == 2 ? 0xff : 0x00);
	outputByte(MODXO_REGISTER_LED_DATA, (ledColor & 4) == 4 ? 0xff : 0x00);

	outputByte(MODXO_REGISTER_LED_COMMAND, MODXO_LED_CMD_UPDATE_STRIPS);
}

void modchipModxo::setFixedLedColor(uint8_t ledColor)
{
	do
	{
		Sleep(10);
	}
	while (inputByte(MODXO_REGISTER_LED_COMMAND) == 0);

	outputByte(MODXO_REGISTER_LED_COMMAND, MODXO_LED_CMD_SELECT_COLOR);
	outputByte(MODXO_REGISTER_LED_DATA, ledColor);
}

uint32_t modchipModxo::getSlotCount()
{
	uint32_t size = inputByte(MODXO_REGISTER_SIZE);
	uint32_t totalSlots = 4;
	if (size >= 16)
	{
		totalSlots = 16;
	}
	else if (size >= 8)
	{
		totalSlots = 12;
	}
	else if (size >= 4)
	{
		totalSlots = 8;
	}
	return totalSlots;
}

uint32_t modchipModxo::getFlashSize(bool recovery)
{
	uint32_t size = inputByte(MODXO_REGISTER_SIZE);
	return recovery ? 0 : (size << 20);
}

bool modchipModxo::supportsLed()
{
	return true;
}

bool modchipModxo::supportsLcd()
{
	return true;
}

bool modchipModxo::supportsLcdInfo()
{
	return false;
}

bool modchipModxo::supportsLcdContrast()
{
	return true;
}

bool modchipModxo::supportsRecovery()
{
	return false;
}

void modchipModxo::disableRecovery()
{
}

bool modchipModxo::isValidBankSize(uint32_t size)
{
	return size == (256 * 1024) || size == (512 * 1024) || size == (1024 * 1024);
}

bool modchipModxo::isValidFlashSize(bool recovery, uint32_t size)
{
	return size == getFlashSize(recovery) || size == (1024 * 1024);
}

uint32_t modchipModxo::getBankSize(uint8_t bank)
{
	// Return 0 if tsop bank
	if ((bank & 0xc0) == 0xc0)
	{
		return 0;
	}

	uint32_t flashSizeMb = inputByte(MODXO_REGISTER_SIZE) << 20;
	uint32_t bankSizeKb = 1 << (18 + ((bank & 0xc0) >> 6));
	uint32_t flashOffset = (bank & 0x3f) * bankSizeKb;
	
	// Return 0 if offset greater than flash size
	if ((flashOffset + bankSizeKb) > flashSizeMb)
	{
		return 0;
	}
	
	return bankSizeKb;
}

uint32_t modchipModxo::getBankMemOffset(uint8_t bank)
{
	// Return 0 if tsop bank
	if ((bank & 0xc0) == 0xc0)
	{
		return 0;
	}

	uint32_t flashSizeMb = inputByte(MODXO_REGISTER_SIZE) << 20;
	uint32_t bankSizeKb = 1 << (18 + ((bank & 0xc0) >> 6));
	uint32_t flashOffset = (bank & 0x3f) * bankSizeKb;

	// Return 0 if offset greater than flash size
	if ((flashOffset + bankSizeKb) > flashSizeMb)
	{
		return 0;
	}

	return flashOffset;
}

uint32_t modchipModxo::getBankStartOffset(uint8_t bank)
{
	return 0;
}

uint8_t modchipModxo::getBankFromIdAndSlots(uint8_t id, uint8_t slots)
{
	uint32_t flashSizeMb = inputByte(MODXO_REGISTER_SIZE) << 20;
	uint32_t bankSizeKb = slots << 18;
	uint32_t idOffset = 4 / slots;
	uint8_t tempSlots = slots >> 1;
	uint8_t tempId = (uint8_t)((id / slots) + idOffset);
	uint32_t flashOffset = tempId * bankSizeKb;

	// Return 0 if offset greater than flash size
	if ((flashOffset + bankSizeKb) > flashSizeMb)
	{
		return 0;
	}

	return (tempSlots << 6) | tempId;
}

utils::dataContainer* modchipModxo::readBank(uint8_t bank)
{
	setBank(bank); 
	uint32_t bankSize = getBankSize(bank);
	utils::dataContainer* dataContainer = new utils::dataContainer(bankSize);
    volatile uint8_t* lpcMemMap = (volatile uint8_t *)(LPC_MEMORY_BASE + getBankStartOffset(bank));
    memcpy(dataContainer->data, (void*)&lpcMemMap[0], bankSize);
	setBank(MODXO_BANK_BOOTLOADER);
	return dataContainer;
}

void modchipModxo::eraseBank(uint8_t bank)
{
	setBank(bank);

	setLedColor(LED_COLOR_AMBER);

	volatile uint8_t* lpcMemMap = (volatile uint8_t *)LPC_MEMORY_BASE;

	uint32_t memOffset = getBankMemOffset(bank);
	uint32_t startOffset = getBankStartOffset(bank);
	uint32_t bankSize = getBankSize(bank);

	uint32_t offset = 0;
    while (offset < bankSize)
	{
		if (isEraseMemOffset(memOffset + startOffset + offset))
		{
			sectorErase(offset);
		}
		offset += 4096;
	}

	setBank(MODXO_BANK_BOOTLOADER);

#ifndef TOOLS
	setLedColor(settingsManager::getLedColor());
#else
	setLedColor(LED_COLOR_GREEN);
#endif
}

void modchipModxo::writeBank(uint8_t bank, utils::dataContainer* dataContainer)
{
	setBank(bank);

	setLedColor(LED_COLOR_BLUE);

	volatile uint8_t* lpcMemMap = (volatile uint8_t *)(LPC_MEMORY_BASE + getBankStartOffset(bank));

	uint32_t memOffset = getBankMemOffset(bank);

	bool needsFlush = false;
	uint8_t sector = 0;
    for (uint32_t i = 0; i < dataContainer->size; i++)
	{
		if (isProtectedMemOffset(memOffset + i) == true)
		{
			continue;
		}
		needsFlush = true;
		uint8_t value = (uint8_t)dataContainer->data[i];
		lpcMemMap[i] = value;
		if (needsFlush && (i & 0xfff) == 0xfff)
		{
			needsFlush = false;
			outputByte(MODXO_REGISTER_MEM_FLUSH, sector);
			do
			{
				Sleep(2);
			}
			while(inputByte(MODXO_REGISTER_MEM_FLUSH) != 0);
			sector++;
		}
    }

	if (needsFlush)
	{
		outputByte(MODXO_REGISTER_MEM_FLUSH, sector);
		do
		{
			Sleep(2);
		}
		while(inputByte(MODXO_REGISTER_MEM_FLUSH) != 0);
	}

	setBank(MODXO_BANK_BOOTLOADER);

#ifndef TOOLS
	setLedColor(settingsManager::getLedColor());
#else
	setLedColor(LED_COLOR_GREEN);
#endif
}

bool modchipModxo::verifyBank(uint8_t bank, utils::dataContainer* dataContainer)
{
	setBank(bank);

	utils::dataContainer* writtenData = readBank(bank);

	setLedColor(LED_COLOR_PURPLE);

	volatile uint8_t* lpcMemMap = (volatile uint8_t *)LPC_MEMORY_BASE;

	uint32_t memOffset = getBankMemOffset(bank);

	bool ok = true;
    for (uint32_t i = 0; i < dataContainer->size; i++)
	{
		if (isProtectedMemOffset(memOffset + i) == true)
		{
			continue;
		}
		if (writtenData->data[i] == dataContainer->data[i])
		{
			continue;
		}
		ok = false;
		break;
    }

	delete(writtenData);

	setBank(MODXO_BANK_BOOTLOADER);

#ifndef TOOLS
	setLedColor(settingsManager::getLedColor());
#else
	setLedColor(LED_COLOR_GREEN);
#endif
	return ok;
}

uint8_t modchipModxo::getFlashBankCount(bool recovery)
{
	uint8_t size = inputByte(MODXO_REGISTER_SIZE);
	return recovery ? 0 : size;
}

uint8_t modchipModxo::getFlashBank(bool recovery, uint8_t bank)
{
	return recovery ? 0 : (bank | 0x80);
}

bankType modchipModxo::getFlashBankType(bool recovery, uint8_t bank)
{
	return recovery ? bankTypeUser : (bank == 0 ? bankTypeSystem : bankTypeUser);
}

utils::dataContainer* modchipModxo::readFlash(bool recovery)
{
	utils::dataContainer* result = new utils::dataContainer(getFlashSize(recovery));
	for (uint8_t i = 0; i < getFlashBankCount(recovery); i++)
	{
		uint8_t bank = getFlashBank(recovery, i);
		uint32_t memOffset = getBankMemOffset(bank);
		utils::dataContainer* bankData = readBank(bank);
		memcpy(result->data + memOffset, bankData->data, bankData->size);
		delete(bankData);
	}
	return result;
}

void modchipModxo::launchBank(uint8_t bank, uint8_t ledColor)
{
	DWORD scratch;
	HalReadSMBusByte(SMBDEV_PIC16L, PIC16L_CMD_SCRATCH_REGISTER, &scratch);
	HalWriteSMBusByte(SMBDEV_PIC16L, PIC16L_CMD_SCRATCH_REGISTER, scratch & ~SCRATCH_REGISTER_BITVALUE_NO_ANIMATION);
	setBank(bank);
	setFixedLedColor(ledColor);
	HalReturnToFirmware(RETURN_FIRMWARE_REBOOT);
}

void modchipModxo::launchTsop()
{
	DWORD scratch;
	HalReadSMBusByte(SMBDEV_PIC16L, PIC16L_CMD_SCRATCH_REGISTER, &scratch);
	HalWriteSMBusByte(SMBDEV_PIC16L, PIC16L_CMD_SCRATCH_REGISTER, scratch & ~SCRATCH_REGISTER_BITVALUE_NO_ANIMATION);
	setBank(MODXO_BANK_TSOP);
	setFixedLedColor(LED_COLOR_OFF);
	HalReturnToFirmware(RETURN_FIRMWARE_REBOOT);
}

void modchipModxo::launchRecovery()
{
}

#ifndef TOOLS

void modchipModxo::loadSettings(settingsState& settings)
{
	setBank(MODXO_SETTINGS_BANK); 

	setLedColor(LED_COLOR_WHITE);

    volatile uint8_t* lpcMemMap = (volatile uint8_t *)LPC_MEMORY_BASE;

    memcpy(&settings, (void*)&lpcMemMap[MODXO_SETTINGS_OFFSET], sizeof(settings));
	uint32_t checksum = crc32::calculate(((uint8_t*)&settings) + sizeof(uint32_t), sizeof(settings) - sizeof(uint32_t));

	setBank(MODXO_BANK_BOOTLOADER);

	if (checksum != settings.checksum || versioning::compareVersion(settings.version, settingsManager::getVersion()) != 0)
	{
		settingsManager::initSettings();
		settingsManager::saveSettings();
	}

	setLedColor(settingsManager::getLedColor());
}

void modchipModxo::saveSettings(settingsState settings) 
{
	setBank(MODXO_SETTINGS_BANK); 

	settings.checksum = crc32::calculate(((uint8_t*)&settings) + sizeof(uint32_t), sizeof(settings) - sizeof(uint32_t));
	utils::dataContainer* settingsData = new utils::dataContainer((char*)&settings, sizeof(settings), sizeof(settings));

	volatile uint8_t* lpcMemMap = (volatile uint8_t *)LPC_MEMORY_BASE;

	setLedColor(LED_COLOR_AMBER);
	sectorErase(MODXO_SETTINGS_OFFSET);

	setLedColor(LED_COLOR_BLUE);

	bool needsFlush = false;
	uint8_t sector = (uint8_t)(MODXO_SETTINGS_OFFSET >> 12);
    for (uint32_t i = 0; i < settingsData->size; i++)
	{
		needsFlush = true;
		uint8_t value = (uint8_t)settingsData->data[i];
		lpcMemMap[MODXO_SETTINGS_OFFSET + i] = value;
		if (needsFlush && ((MODXO_SETTINGS_OFFSET + i) & 0xfff) == 0xfff)
		{
			needsFlush = false;
			outputByte(MODXO_REGISTER_MEM_FLUSH, sector);
			do
			{
				Sleep(2);
			}
			while(inputByte(MODXO_REGISTER_MEM_FLUSH) != 0);
			sector++;
		}
    }

	if (needsFlush)
	{
		outputByte(MODXO_REGISTER_MEM_FLUSH, sector);
		do
		{
			Sleep(2);
		}
		while(inputByte(MODXO_REGISTER_MEM_FLUSH) != 0);
	}

	setBank(MODXO_BANK_BOOTLOADER);

	setLedColor(settingsManager::getLedColor());
}

#endif

utils::dataContainer* modchipModxo::getInstallerLogo()
{
	utils::dataContainer* installerLogo = new utils::dataContainer(32768);
	return installerLogo;
}

void modchipModxo::lcdSendCharacter(uint8_t value, uint8_t command)
{
	if (command == 0)
	{
		outputByte(MODXO_REGISTER_LCD_DATA, value);
	}
	else
	{
		outputByte(MODXO_REGISTER_LCD_COMMAND, value);
	}
}

void modchipModxo::lcdSetCursorPosition(uint8_t row, uint8_t col)
{
	if (row > 3) 
	{
		row = 3;
	}
	if (col > 19)
	{
		col = 19; 
	}
	utils::debugPrint("PromSetCursor row=%i col=%i\n", row, col);
	lcdSendCharacter(MODXO_TD_SET_CURSOR_POSITION, 1);
	lcdSendCharacter(row, 1);
	lcdSendCharacter(col, 1);
	Sleep(MODXO_LCD_DELAY);
}

uint8_t modchipModxo::getLcdTypeCount()
{
	return 2;
}

char* modchipModxo::getLcdTypeString(uint8_t lcdEnableType)
{
	if (lcdEnableType == 1)
	{
		return strdup("I2C");
	}
	
	return strdup("Disabled");
}

void modchipModxo::lcdInit(uint8_t backlight, uint8_t contrast)
{
	// init display
	lcdSendCharacter(MODXO_TD_INIT, 1); 
	Sleep(MODXO_LCD_DELAY);

	lcdSetBacklight(backlight);
	lcdSetContrast(contrast);
	Sleep(MODXO_LCD_DELAY);
}

void modchipModxo::lcdPrintMessage(const char* message)
{
	for (int i = 0; i < (int)strlen(message); ++i)
	{
		uint8_t cLCD = message[i];
		lcdSendCharacter(cLCD, 0);
		Sleep(MODXO_LCD_DELAY);
	}
}

void modchipModxo::lcdSetBacklight(uint8_t value)
{
	if (value > 100)
	{
		value = 100;
	}
	lcdSendCharacter(MODXO_TD_SET_BACKLIGHT, 1);
	lcdSendCharacter(value, 1);
	Sleep(MODXO_LCD_DELAY);
}

void modchipModxo::lcdSetContrast(uint8_t value)
{
	if (value > 100)
	{
		value = 100;
	}
	lcdSendCharacter(MODXO_TD_SET_CONTRAST, 1);
	lcdSendCharacter(value, 1);
	Sleep(MODXO_LCD_DELAY);
}

// Private

void modchipModxo::setBank(uint8_t bank)
{
	mBank = bank;
    outputByte(MODXO_REGISTER_BANKING, mBank);
}

uint8_t modchipModxo::getBank()
{
	mBank = (inputByte(MODXO_REGISTER_BANKING) & 0x0f);
	return mBank;
}

bool modchipModxo::isEraseMemOffset(uint32_t memOffset)
{
	return (memOffset % 4096) == 0;
}

bool modchipModxo::isProtectedMemOffset(uint32_t memOffset)
{
	return false;
}

void modchipModxo::sectorErase(uint32_t offset)
{
	if (isProtectedMemOffset(offset))
	{
		return;
	}

	outputByte(MODXO_REGISTER_MEM_ERASE, 'D');
	outputByte(MODXO_REGISTER_MEM_ERASE, 'I');
	outputByte(MODXO_REGISTER_MEM_ERASE, 'E');
	outputByte(MODXO_REGISTER_MEM_ERASE, (uint8_t)(offset >> 12));
	do
	{
		Sleep(2);
	}
	while(inputByte(MODXO_REGISTER_MEM_ERASE) != 0);
}