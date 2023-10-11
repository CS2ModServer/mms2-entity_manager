#include "gamedata.h"

#include <stdlib.h>

#include <tier1/utlstring.h>

#include <entity2/entitysystem.h>
#include <tier0/dbg.h>
#include <tier0/platform.h>
#include <tier1/KeyValues.h>

#define GAMECONFIG_FOLDER_DIR "gamedata"
#define GAMECONFIG_ENTITY_SYSTEM_FILENAME "entity_system.games.txt"
#define GAMECONFIG_ENTITY_KEYVALUES_FILENAME "entity_keyvalues.games.txt"

extern IVEngineServer *engine;
extern IFileSystem *filesystem;
extern IServerGameDLL *server;
extern CGameEntitySystem *g_pEntitySystem;

extern EntityManagerSpace::GameData *g_pEntityManagerGameData;

CModule g_aLibEngine, 
        g_aLibServer;

bool EntityManagerSpace::GameData::Init(char *psError, size_t nMaxLength)
{
	bool bResult = g_aLibEngine.InitFromMemory(engine);

	if(bResult)
	{
		this->m_aLibraryMap["engine"] = &g_aLibEngine;

		bResult = g_aLibServer.InitFromMemory(server);

		if(bResult)
		{
			this->m_aLibraryMap["server"] = &g_aLibServer;
		}
		else
		{
			strncpy(psError, "Failed to get server module", nMaxLength);
		}
	}
	else
	{
		strncpy(psError, "Failed to get engine module", nMaxLength);
	}

	return true;
}

bool EntityManagerSpace::GameData::Load(const char *pszBasePath, char *psError, size_t nMaxLength)
{
	char sBaseGameConfigDir[MAX_PATH];

	snprintf((char *)sBaseGameConfigDir, sizeof(sBaseGameConfigDir), 
#ifdef PLATFORM_WINDOWS
		"%s\\%s", 
#else
		"%s/%s", 
#endif
		pszBasePath, GAMECONFIG_FOLDER_DIR);

	bool bResult = this->LoadEntityKeyValues((const char *)sBaseGameConfigDir, psError, nMaxLength);

	if(bResult)
	{
		bResult = this->LoadEntitySystem((const char *)sBaseGameConfigDir, psError, nMaxLength);
	}

	return bResult;
}

void EntityManagerSpace::GameData::Clear()
{
	this->m_aEntitySystemConfig.Clear();
}

void EntityManagerSpace::GameData::Destroy()
{
	// ...
}

const CModule *EntityManagerSpace::GameData::FindLibrary(const char *pszName)
{
	auto itResult = this->m_aLibraryMap.find(pszName);

	return itResult == this->m_aLibraryMap.cend() ? nullptr : itResult->second;
}

bool EntityManagerSpace::GameData::LoadEntityKeyValues(const char *pszBaseGameConfigDir, char *psError, size_t nMaxLength)
{
	char sConfigFile[MAX_PATH];

	snprintf((char *)sConfigFile, sizeof(sConfigFile), 
#ifdef PLATFORM_WINDOWS
		"%s\\%s",
#else
		"%s/%s", 
#endif
		pszBaseGameConfigDir, GAMECONFIG_ENTITY_KEYVALUES_FILENAME);

	KeyValues *pGamesValues = new KeyValues("Games");

	bool bResult = pGamesValues->LoadFromFile(filesystem, (const char *)sConfigFile);

	if(bResult)
	{
		char sGameConfigError[1024];

		bResult = this->m_aEntityKeyValuesConfig.Load(pGamesValues, (char *)sGameConfigError, sizeof(sGameConfigError));

		if(!bResult && psError)
		{
			snprintf(psError, nMaxLength, "Failed to load a entity keyvalues: %s", sGameConfigError);
		}
	}
	else if(psError)
	{
		snprintf(psError, nMaxLength, "Can't to load KeyValue from \"%s\" file", sConfigFile);
	}

	delete pGamesValues;

	return bResult;
}

bool EntityManagerSpace::GameData::LoadEntitySystem(const char *pszBaseGameConfigDir, char *psError, size_t nMaxLength)
{
	char sConfigFile[MAX_PATH];

	snprintf((char *)sConfigFile, sizeof(sConfigFile), 
#ifdef PLATFORM_WINDOWS
		"%s\\%s",
#else
		"%s/%s", 
#endif
		pszBaseGameConfigDir, GAMECONFIG_ENTITY_SYSTEM_FILENAME);

	KeyValues *pGamesValues = new KeyValues("Games");

	bool bResult = pGamesValues->LoadFromFile(filesystem, (const char *)sConfigFile);

	if(bResult)
	{
		char sGameConfigError[1024];

		bResult = this->m_aEntitySystemConfig.Load(pGamesValues, (char *)sGameConfigError, sizeof(sGameConfigError));

		if(!bResult && psError)
		{
			snprintf(psError, nMaxLength, "Failed to load a entity system: %s", sGameConfigError);
		}
	}
	else if(psError)
	{
		snprintf(psError, nMaxLength, "Can't to load KeyValue from \"%s\" file", sConfigFile);
	}

	delete pGamesValues;

	return bResult;
}

CMemory EntityManagerSpace::GameData::GetEntityKeyValuesAddress(const std::string &sName)
{
	return this->m_aEntityKeyValuesConfig.GetAddress(sName);
}

ptrdiff_t EntityManagerSpace::GameData::GetEntityKeyValuesOffset(const std::string &sName)
{
	return this->m_aEntityKeyValuesConfig.GetOffset(sName);
}

CMemory EntityManagerSpace::GameData::GetEntitySystemAddress(const std::string &sName)
{
	return this->m_aEntitySystemConfig.GetAddress(sName);
}

ptrdiff_t EntityManagerSpace::GameData::GetEntitySystemOffset(const std::string &sName)
{
	return this->m_aEntitySystemConfig.GetOffset(sName);
}

const char *EntityManagerSpace::GameData::GetSourceEngineName()
{
#if SOURCE_ENGINE == SE_CS2
	return "cs2";
#elif SOURCE_ENGINE == SE_DOTA
	return "dota";
#else
#	error "Unknown engine type"
	return "unknown";
#endif
}

const char *EntityManagerSpace::GameData::GetPlatformName()
{
#if defined(_WINDOWS)
#	if defined(X64BITS)
	return "windows64";
#	else
	return "windows";
#	endif
#elif defined(_LINUX)
#	if defined(X64BITS)
	return "linux64";
#	else
	return "linux";
#	endif
#else
#	error Unsupported platform
	return "unknown";
#endif
}

ptrdiff_t EntityManagerSpace::GameData::ReadOffset(const char *pszValue)
{
	return static_cast<ptrdiff_t>(strtol(pszValue, NULL, 0));
}

bool EntityManagerSpace::GameData::Config::Load(KeyValues *pGamesValues, char *psError, size_t nMaxLength)
{
	const char *pszEngineName = EntityManagerSpace::GameData::GetSourceEngineName();

	KeyValues *pEngineValues = pGamesValues->FindKey(pszEngineName, false);

	bool bResult = pEngineValues != nullptr;

	if(bResult)
	{
		this->LoadEngine(pEngineValues, psError, nMaxLength);
	}
	else if(!psError)
	{
		snprintf(psError, nMaxLength, "Failed to find \"%s\" section", pszEngineName);
	}

	return bResult;
}

bool EntityManagerSpace::GameData::Config::LoadEngine(KeyValues *pEngineValues, char *psError, size_t nMaxLength)
{
	KeyValues *pSectionValues = pEngineValues->FindKey("Signatures", false);

	bool bResult = true;

	if(pSectionValues) // Ignore the section not found for result.
	{
		bResult = this->LoadEngineSignatures(pSectionValues, psError, nMaxLength);

		if(bResult)
		{
			pSectionValues = pEngineValues->FindKey("Offsets", false);

			if(pSectionValues) // Same ignore.
			{
				bResult = bResult = this->LoadEngineOffsets(pSectionValues, psError, nMaxLength);
			}
		}
	}

	return bResult;
}

bool EntityManagerSpace::GameData::Config::LoadEngineSignatures(KeyValues *pSignaturesValues, char *psError, size_t nMaxLength)
{
	KeyValues *pSigSection = pSignaturesValues->GetFirstSubKey();

	bool bResult = pSigSection != nullptr;

	if(bResult)
	{
		const char *pszLibraryKey = "library", 
		           *pszPlatformKey = EntityManagerSpace::GameData::GetPlatformName();

		do
		{
			const char *pszSigName = pSigSection->GetName();

			KeyValues *pLibraryValues = pSigSection->FindKey(pszLibraryKey, false);

			bResult = pLibraryValues != nullptr;

			if(bResult)
			{
				const char *pszLibraryName = pLibraryValues->GetString(nullptr, "unknown");

				const CModule *pLibModule = g_pEntityManagerGameData->FindLibrary(pszLibraryName);

				bResult = (bool)pLibModule;

				if(bResult)
				{
					KeyValues *pPlatformValues = pSigSection->FindKey(pszPlatformKey, false);

					bResult = pPlatformValues != nullptr;

					if(pPlatformValues)
					{
						const char *pszSignature = pPlatformValues->GetString(nullptr);

						CMemory pSigResult = pLibModule->FindPatternSIMD(pszSignature);

						bResult = (bool)pSigResult;

						if(bResult)
						{
							Msg("pszSigName = %s (%p)\n", pszSigName, pSigResult);
							this->SetAddress(pszSigName, pSigResult);
						}
						else if(psError)
						{
							snprintf(psError, nMaxLength, "Failed to find \"%s\" signature", pszSigName);
						}
					}
					else if(psError)
					{
						snprintf(psError, nMaxLength, "Failed to get platform (\"%s\" key) at \"%s\" signature", pszPlatformKey, pszSigName);
					}
				}
				else if(psError)
				{
					snprintf(psError, nMaxLength, "Unknown \"%s\" library at \"%s\" signature", pszLibraryName, pszSigName);
				}
			}
			else if(psError)
			{
				snprintf(psError, nMaxLength, "Failed to get library (\"%s\" key) at \"%s\" signature", pszLibraryKey, pszSigName);
			}

			pSigSection = pSigSection->GetNextKey();
		}
		while(pSigSection);
	}
	else if(psError)
	{
		strncpy(psError, "Signatures section is empty", nMaxLength);
	}

	return bResult;
}

bool EntityManagerSpace::GameData::Config::LoadEngineOffsets(KeyValues *pOffsetsValues, char *psError, size_t nMaxLength)
{
	KeyValues *pOffsetSection = pOffsetsValues->GetFirstSubKey();

	bool bResult = pOffsetSection != nullptr;

	if(bResult)
	{
		const char *pszPlatformKey = EntityManagerSpace::GameData::GetPlatformName();

		do
		{
			const char *pszOffsetName = pOffsetSection->GetName();

			KeyValues *pPlatformValues = pOffsetSection->FindKey(pszPlatformKey, false);

			bResult = pPlatformValues != nullptr;

			if(pPlatformValues)
			{
				Msg("pszOffsetName = %s\n", pszOffsetName);
				this->SetOffset(pszOffsetName, EntityManagerSpace::GameData::ReadOffset(pPlatformValues->GetString(NULL)));
			}
			else if(psError)
			{
				snprintf(psError, nMaxLength, "Failed to get platform (\"%s\" key) at \"%s\" signature", pszPlatformKey, pszOffsetName);
			}

			pOffsetSection = pOffsetSection->GetNextKey();
		}
		while(pOffsetSection);
	}
	else if(psError)
	{
		strncpy(psError, "Offsets section is empty", nMaxLength);
	}

	return bResult;
}


CMemory EntityManagerSpace::GameData::Config::GetAddress(const std::string &sName) const
{
	auto itResult = this->m_aAddressMap.find(sName);

	if(itResult != this->m_aAddressMap.cend())
	{
		CMemory pResult = itResult->second;

		DebugMsg("Address (or signature) \"%s\" is %p\n", sName.c_str(), (void *)pResult);

		return pResult;
	}

	DevWarning("Address (or signature) \"%s\" is not found\n", sName.c_str());

	return nullptr;
}


ptrdiff_t EntityManagerSpace::GameData::Config::GetOffset(const std::string &sName) const
{
	auto itResult = this->m_aOffsetMap.find(sName);

	if(itResult != this->m_aOffsetMap.cend())
	{
		ptrdiff_t nResult = itResult->second;

		DebugMsg("Offset \"%s\" is 0x%zX (%zd)\n", sName.c_str(), nResult, nResult);

		return nResult;
	}

	DevWarning("Offset \"%s\" is not found\n", sName.c_str());

	return -1;
}

void EntityManagerSpace::GameData::Config::SetAddress(const std::string &sName, const CMemory &aMemory)
{
	this->m_aAddressMap[sName] = aMemory;
}

void EntityManagerSpace::GameData::Config::SetOffset(const std::string &sName, const ptrdiff_t nValue)
{
	this->m_aOffsetMap[sName] = nValue;
}

void EntityManagerSpace::GameData::Config::Clear()
{
	this->m_aAddressMap.clear();
	this->m_aOffsetMap.clear();
}
