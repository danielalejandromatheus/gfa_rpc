// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <thread> // For sleep
#include <vector>
#include <tlhelp32.h>
#include "proc.h"
#include <tchar.h>
#include <stdlib.h>
#include <string>
#include <psapi.h>
#include <fstream>
#include <sstream>
#include "discord/discord.h"
const wchar_t* targetProcessName = L"GrandFantasia.exe";
const TCHAR tTargetProcessName[] = _T("GrandFantasia.exe");
const DWORD targetPlayerNameAddr = 0x00975748;
const DWORD targetPlayerCharLvAddr = 0x0097C48C;
const DWORD targetPlayerPartySizeAddr = 0x0097574C;
const DWORD targetZoneNodeAddr = 0x0097574C;
const DWORD targetZoneNameAddr = 0x00977360;
const DWORD targetPlayerClassBaseAddr = 0x0097C48C;
const DWORD targetPlayerChAddr = 0x0097594C;
std::vector<DWORD> targetLvOffsets = { 0x40, 0x10, 0xBC, 0x5C };
std::vector<DWORD> nameOffsets = { 0x60,0x4,0x64,0xA8,0x24,0x41C };
std::vector<DWORD> zoneNodeOffsets = { 0xD0,0x8,0x10,0x8,0x200 };
std::vector<DWORD> classNodeOffsets = { 0x40,0x10,0x138,0xC,0x24, 0xD4, 0x1C };
std::vector<DWORD> partySizeOffsets = { 0x34, 0xC8, 0x8, 0x10, 0x8, 0x428 };
std::vector<DWORD> partyMaxOffsets = { 0xD0, 0x8, 0x4, 0x8, 0x10, 0x8, 0x2DC };
std::vector<DWORD> channelOffsets = { 0x140, 0x64, 0xAC, 0x24, 0x250 };
std::vector<DWORD> zoneNameOffsets = { 0xC0, 0xC, 0xC, 0x24, 0xBC, 0x5C, 0x0 };
std::vector<DWORD> zoneNameFallbackOffsets = { 0xD4, 0xC, 0xC, 0xC, 0x24, 0xBC, 0x5C };
DWORD procId = 0;
const char locale[4] = "US";
bool isValidUTF8(const std::string& input) {
	size_t i = 0;
	const size_t length = input.size();

	while (i < length) {
		unsigned char c = static_cast<unsigned char>(input[i]);

		if (c <= 0x7F) {
			// Single-byte character (ASCII)
			i++;
		}
		else if ((c & 0xE0) == 0xC0) {
			// Two-byte sequence (110xxxxx 10xxxxxx)
			if (i + 1 >= length || (input[i + 1] & 0xC0) != 0x80) {
				return false;
			}
			unsigned int codepoint = ((c & 0x1F) << 6) | (input[i + 1] & 0x3F);
			if (codepoint < 0x80) return false; // Overlong encoding
			i += 2;
		}
		else if ((c & 0xF0) == 0xE0) {
			// Three-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
			if (i + 2 >= length ||
				(input[i + 1] & 0xC0) != 0x80 ||
				(input[i + 2] & 0xC0) != 0x80) {
				return false;
			}
			unsigned int codepoint = ((c & 0x0F) << 12) |
				((input[i + 1] & 0x3F) << 6) |
				(input[i + 2] & 0x3F);
			if (codepoint < 0x800) return false; // Overlong encoding
			i += 3;
		}
		else if ((c & 0xF8) == 0xF0) {
			// Four-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
			if (i + 3 >= length ||
				(input[i + 1] & 0xC0) != 0x80 ||
				(input[i + 2] & 0xC0) != 0x80 ||
				(input[i + 3] & 0xC0) != 0x80) {
				return false;
			}
			unsigned int codepoint = ((c & 0x07) << 18) |
				((input[i + 1] & 0x3F) << 12) |
				((input[i + 2] & 0x3F) << 6) |
				(input[i + 3] & 0x3F);
			if (codepoint < 0x10000 || codepoint > 0x10FFFF) return false; // Invalid range
			i += 4;
		}
		else {
			// Invalid first byte
			return false;
		}
	}

	return true;
}
struct DiscordState {
	std::unique_ptr<discord::Core> core;
};
struct ClassData {
	int classNode = -1;
	std::string className;
};

std::vector<ClassData> ENClassDictionary = {
	{0, "Novice"},
	{1, "Fighter"},
	{2, "Warrior"},
	{3, "Berserker"},
	{4, "Paladin"},
	{5, "Hunter"},
	{6, "Archer"},
	{7, "Ranger"},
	{8, "Assassin"},
	{9, "Acolyte"},
	{10, "Priest"},
	{11, "Cleric"},
	{12, "Sage"},
	{13, "Spellcaster"},
	{14, "Mage"},
	{15, "Wizard"},
	{16, "Necromancer"},
	{17, "Warlord"},
	{18, "Templar"},
	{19, "Sharpshooter"},
	{20, "Darkstalker"},
	{21, "Prophet"},
	{22, "Mystic"},
	{23, "Archmage"},
	{24, "Demonologist"},
	{32, "Deathknight"},
	{33, "Crusader"},
	{34, "Hawkeye"},
	{35, "Windshadow"},
	{36, "Saint"},
	{37, "Shaman"},
	{38, "Avatar"},
	{39, "Shadowlord"},
	{40, "Destroyer"},
	{41, "Holy Knight"},
	{42, "Predator"},
	{43, "Shinobi"},
	{44, "Archangel"},
	{45, "Druid"},
	{46, "Warlock"},
	{47, "Shinigami"},
	{25, "Duskwraith"},
	{26, "Guardian of Shadows"},
	{27, "Soul Reaver"},
	{28, "Dark Knight"},
	{29, "Wraithblade"},
	{30, "Abyssal Knight"},
	{48, "Wraithblade"},
	{49, "Living Shadow"},
	{50, "Eclipse Reaver"},
	{51, "Shadowbringer"},
	{52, "Wanderer"},
	{53, "Drifter"},
	{54, "Void Runner"},
	{55, "Time Traveler"},
	{56, "Dimensionalist"},
	{57, "Key Master"},
	{58, "Soul Reaver"},
	{59, "Chronomancer"},
	{60, "Phantom"},
	{61, "Chronoshifter"}
};
std::vector<ClassData> ESClassDictionary = {
	{0, "Novato"},
	{1, "Luchador"},
	{2, "Guerrero"},
	{3, "Berserker"},
	{4, "Paladin"},
	{5, "Cazador"},
	{6, "Arquero"},
	{7, "Ranger"},
	{8, "Asesino"},
	{9, "Acólito"},
	{10, "Sacerdote"},
	{11, "Clérigo"},
	{12, "Sabio"},
	{13, "Hechicero"},
	{14, "Mago"},
	{15, "Mago Brujo"},
	{16, "Nigromante"},
	{17, "Dios de Guerra"},
	{18, "Templario"},
	{19, "Francotirador"},
	{20, "Acechador Oscuro"},
	{21, "Profeta"},
	{22, "Místico"},
	{23, "Archimago"},
	{24, "Demonólogo"},
	{32, "Caballero de la Muerte"},
	{33, "Caballero Sagrado"},
	{34, "Ballestero Mortal"},
	{35, "Ninja Fantasmal"},
	{36, "Santo"},
	{37, "Chamán"},
	{38, "Avatar"},
	{39, "Amo de Almas"},
	{40, "Destructor"},
	{41, "Cruzado"},
	{42, "Depredador"},
	{43, "Shinobi"},
	{44, "Arcángel"},
	{45, "Druida"},
	{46, "Brujo"},
	{47, "Shinigami"},
	{25, "Sombrío"},
	{26, "Guardián de las Sombras"},
	{27, "Segador de Almas"},
	{28, "Caballero Oscuro"},
	{29, "Espada Espectral"},
	{30, "Caballero Abismal"},
	{48, "Precursor de Almas"},
	{49, "Sombra Viviente"},
	{50, "Segador del Eclipse"},
	{51, "Portador de Sombras"},
	{52, "Viajero"},
	{53, "Nómada"},
	{54, "Duelista"},
	{55, "Relojero"},
	{56, "Samurai"},
	{57, "Maestro del Tiempo"},
	{58, "San La Muerte"},
	{59, "Oráculo"},
	{60, "Fantasmal"},
	{61, "Alterador Cósmico"}
};
std::vector<ClassData> PTClassDictionary = {
	{0, "Novato"},
	{1, "Lutador"},
	{2, "Guerreiro"},
	{3, "Berserker"},
	{4, "Paladino"},
	{5, "Caçador"},
	{6, "Arqueiro"},
	{7, "Ranger"},
	{8, "Assassino"},
	{9, "Acólito"},
	{10, "Sacerdote"},
	{11, "Clérigo"},
	{12, "Sábio"},
	{13, "Bruxo"},
	{14, "Mago"},
	{15, "Feiticeiro"},
	{16, "Necromante"},
	{17, "Titã"},
	{18, "Templário"},
	{19, "Franco Atirador"},
	{20, "Sicário Sombrio"},
	{21, "Profeta"},
	{22, "Místico"},
	{23, "Arquimago"},
	{24, "Demonólogo"},
	{32, "Cavaleiro da Morte"},
	{33, "Cavaleiro Real"},
	{34, "Mercenário"},
	{35, "Ninja"},
	{36, "Mensageiro Divino"},
	{37, "Xamã"},
	{38, "Arcano"},
	{39, "Emissário dos Mortos"},
	{40, "Destruidor"},
	{41, "Cavaleiro Sagrado"},
	{42, "Predador"},
	{43, "Shinobi"},
	{44, "Arcanjo"},
	{45, "Druida"},
	{46, "Bruxo"},
	{47, "Shinigami"},
	{25, "Sombrio"},
	{26, "Guardião das Sombras"},
	{27, "Ceifador de Almas"},
	{28, "Cavaleiro das Trevas"},
	{29, "Lâmina Espectral"},
	{30, "Cavaleiro Abissal"},
	{48, "Arauto das Almas"},
	{49, "Sombra Viva"},
	{50, "Ceifador do Eclipse"},
	{51, "Portador das Sombras"},
	{52, "Viajante"},
	{53, "Nómade"},
	{54, "Espadachim"},
	{55, "Ilusionista"},
	{56, "Samurai"},
	{57, "Augure"},
	{58, "Ronin"},
	{59, "Oráculo"},
	{60, "Mestre Dimensional"},
	{61, "Cronos"}
};
struct CharData {
	char name[20] = "";
	char level[4] = "";
	short zoneNode = -1;
	char zoneName[40] = "";
	short classNode = -1;
	char className[20] = "";
	char classKey[20] = "";
	char classAndLevel[30] = "";
	short partySize = -1;
	short partyMax = -1;
	char channel[255] = "";
	char zoneUrl[255] = "";
	bool isCharacterLoggedOut() {
		return zoneNode == 99 || zoneNode == -1 && classNode == -1 && partySize == -1 && partyMax == -1;
	}
	bool isCharDataEmpty() {
		return std::strcmp(level, "") == 0 && classNode < 1 && partySize < 1 && partyMax < 1;
	}
};
CharData getCharData(HANDLE hProcess, DWORD gameBaseAddress) {
	CharData charData;
	DWORD LvPtrAddr = GetPointerAddress(hProcess, gameBaseAddress, targetPlayerCharLvAddr, targetLvOffsets);
	ReadProcessMemory(hProcess, (BYTE*)LvPtrAddr, &charData.level, sizeof(charData.level), nullptr);
	DWORD NamePtrAddr = GetPointerAddress(hProcess, gameBaseAddress, targetPlayerNameAddr, nameOffsets);
	ReadProcessMemory(hProcess, (BYTE*)NamePtrAddr, &charData.name, sizeof(charData.name), nullptr);
	DWORD ZoneNodePtrAddr = GetPointerAddress(hProcess, gameBaseAddress, targetZoneNodeAddr, zoneNodeOffsets);
	ReadProcessMemory(hProcess, (BYTE*)ZoneNodePtrAddr, &charData.zoneNode, sizeof(charData.zoneNode), nullptr);
	DWORD ClassNodePtrAddr = GetPointerAddress(hProcess, gameBaseAddress, targetPlayerClassBaseAddr, classNodeOffsets);
	ReadProcessMemory(hProcess, (BYTE*)ClassNodePtrAddr, &charData.classNode, sizeof(charData.classNode), nullptr);
	DWORD PartySizePtrAddr = GetPointerAddress(hProcess, gameBaseAddress, targetPlayerPartySizeAddr, partySizeOffsets);
	ReadProcessMemory(hProcess, (BYTE*)PartySizePtrAddr, &charData.partySize, sizeof(charData.partySize), nullptr);
	DWORD PartyMaxPtrAddr = GetPointerAddress(hProcess, gameBaseAddress, targetPlayerPartySizeAddr, partyMaxOffsets);
	ReadProcessMemory(hProcess, (BYTE*)PartyMaxPtrAddr, &charData.partyMax, sizeof(charData.partyMax), nullptr);
	DWORD ChannelPtrAddr = GetPointerAddress(hProcess, gameBaseAddress, targetPlayerChAddr, channelOffsets);
	ReadProcessMemory(hProcess, (BYTE*)ChannelPtrAddr, &charData.channel, sizeof(charData.channel), nullptr);
	if (charData.zoneNode) {
		DWORD ZoneNamePtrAddr = GetPointerAddress(hProcess, gameBaseAddress, targetZoneNameAddr, zoneNameOffsets);
		ReadProcessMemory(hProcess, (BYTE*)ZoneNamePtrAddr, &charData.zoneName, sizeof(charData.zoneName), nullptr);
		if (std::strcmp(charData.zoneName, "") == 0 || !isValidUTF8(charData.zoneName)) {
			DWORD ZoneNameFallbackPtrAddr = GetPointerAddress(hProcess, gameBaseAddress, targetZoneNameAddr, zoneNameFallbackOffsets);
			ReadProcessMemory(hProcess, (BYTE*)ZoneNameFallbackPtrAddr, &charData.zoneName, sizeof(charData.zoneName), nullptr);
		}
	}
	std::vector<ClassData> classDictionary;
	if (std::strcmp(locale, "ES") == 0) {
		classDictionary = ESClassDictionary;
	}
	else if (std::strcmp(locale, "PT") == 0) {
		classDictionary = PTClassDictionary;
	}
	else {
		classDictionary = ENClassDictionary;
	}
	if (charData.classNode != -1) {
		auto classData = std::find_if(classDictionary.begin(), classDictionary.end(), [charData](ClassData classData) {
			return charData.classNode == classData.classNode;
			});
		if (classData != classDictionary.end()) strcpy_s(charData.className, sizeof(charData.className), classData->className.data());
	}
	char buffer[255] = { 0 };
	if (charData.zoneNode) sprintf_s(buffer, "http://gfawakening.online:8080/node-image/%d", charData.zoneNode);
	strcpy_s(charData.zoneUrl, sizeof(charData.zoneUrl), buffer);
	std::string classKey = "c-" + std::to_string(charData.classNode);
	strcpy_s(charData.classKey, sizeof(charData.classKey), &classKey[0]);
	std::ostringstream imgTextStream;
	imgTextStream << "Lv." << charData.level << " " << charData.className;
	strcpy_s(charData.classAndLevel, sizeof(charData.classAndLevel), imgTextStream.str().c_str());
	return charData;
}
class DiscordRPC {
private:
	CharData charData;

public:
	DiscordState state{};
	discord::Activity activity{};

	DiscordRPC() {
		discord::Core* core{};
		auto result = discord::Core::Create(1290058265098457160, DiscordCreateFlags_Default, &core);
		state.core.reset(core);
		activity.SetType(discord::ActivityType::Playing);
		activity.GetTimestamps().SetStart(time(0));
	}
	~DiscordRPC() {
		state.core.release();
	}
	void setCharData(CharData charData) {
		this->charData = charData;
		//If any value on char data is changed, update the rich presence
		if (
			charData.name != this->charData.name ||
			charData.level != this->charData.level ||
			charData.zoneNode != this->charData.zoneNode ||
			charData.classNode != this->charData.classNode
			) {
			UpdateRichPresence();
		}
	}
	void UpdateRichPresence() {
		if (!charData.isCharacterLoggedOut()) {
			activity.SetDetails(charData.name);
			if (charData.partySize > 1) {
				activity.GetParty().GetSize().SetCurrentSize(charData.partySize);
				activity.GetParty().GetSize().SetMaxSize(charData.partyMax);
			}
			else {
				activity.GetParty().GetSize().SetCurrentSize(0);
				activity.GetParty().GetSize().SetMaxSize(0);
			}
			activity.SetState(charData.channel);
			activity.GetAssets().SetSmallImage(charData.classKey);
			activity.GetAssets().SetSmallText(charData.classAndLevel);
			activity.GetAssets().SetLargeImage(charData.zoneUrl);
			activity.GetAssets().SetLargeText(charData.zoneName);
		}
		else {
			activity.SetDetails("");
			activity.SetState("In Main Menu");
			activity.GetAssets().SetSmallImage("");
			activity.GetAssets().SetSmallText("");
			activity.GetAssets().SetLargeImage("");
			activity.GetAssets().SetLargeText("");
			activity.GetParty().GetSize().SetCurrentSize(0);
			activity.GetParty().GetSize().SetMaxSize(0);
		}
		state.core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
			// Handle the result
			});
		state.core->RunCallbacks();
	}
};
DWORD WINAPI MainThread(HMODULE hModule) {
	DiscordRPC discordRPC;
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
	if (!hProcess || hProcess == INVALID_HANDLE_VALUE) {
		FreeLibraryAndExitThread(hModule, 0);
		return 0;
	}
	DWORD gameBaseAddress = GetModuleBaseAddress(const_cast<TCHAR*>(targetProcessName), procId);
	// check if there is any other process running in the background under the same name, if there is shut down this dll
	while (true) {
		if (GetCurrentProcessId() == 0) {
			CloseHandle(hProcess);
			FreeLibraryAndExitThread(hModule, 0);
		}
		CharData charData = getCharData(hProcess, gameBaseAddress);
		if (!charData.isCharDataEmpty()) {
			discordRPC.setCharData(charData);
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}
BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		procId = GetCurrentProcessId();
		DisableThreadLibraryCalls(hModule); // Avoid thread notifications
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		FreeLibrary(hModule);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

