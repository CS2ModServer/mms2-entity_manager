#include <entity_manager/provider/entitysystem.hpp>
#include <entity_manager/provider/source2server.hpp>
#include <entity_manager/provider/spawngroup.hpp>
#include <entity_manager/provider_agent.hpp>
#include <entity_manager/provider.hpp>

#include <logger.hpp>

#include <ehandle.h> //FIXME: fix this one in https://github.com/Wend4r/hl2sdk/commits/cs2-entity_handle-get
#include <igamesystemfactory.h>
#include <tier0/dbg.h>
#include <tier0/memalloc.h>
#include <tier0/platform.h>
#include <tier0/keyvalues.h>
#include <tier1/generichash.h>
#include <tier1/keyvalues3.h>

extern EntityManager::Provider *g_pEntityManagerProvider;
extern EntityManager::CSpawnGroupAccessor *g_pEntityManagerSpawnGroup;

extern IServerGameDLL *server;
extern IGameEventManager2 *gameeventmanager;

CEntitySystem *g_pEntitySystem = NULL;
CGameEntitySystem *g_pGameEntitySystem = NULL;

CBaseGameSystemFactory **CBaseGameSystemFactory::sm_pFirst = NULL;

CBaseGameSystemFactory *g_pGSFactoryCSpawnGroupMgrGameSystem = NULL;
CSpawnGroupMgrGameSystem *g_pSpawnGroupMgr = NULL;

EntityManager::ProviderAgent::ProviderAgent()
{
}

bool EntityManager::ProviderAgent::Init()
{
	return true;
}

void EntityManager::ProviderAgent::Clear()
{
	ReleaseSpawnQueued();
	ReleaseDestroyQueued();
	ReleaseSpawnGroups();
}

void EntityManager::ProviderAgent::Destroy()
{
	Clear();
}

bool EntityManager::ProviderAgent::NotifyGameResourceUpdated()
{
	bool bResult = m_aResourceManifest.Reinit(RESOURCE_MANIFEST_LOAD_STREAMING_DATA, __FUNCTION__, RESOURCE_MANIFEST_LOAD_PRIORITY_HIGH /* Run-time entities update at players view */);

	if(bResult)
	{
		// ...
	}

	return bResult;
}

bool EntityManager::ProviderAgent::NotifyGameSystemUpdated()
{
	bool bResult = (CBaseGameSystemFactory::sm_pFirst = g_pEntityManagerProvider->GetGameDataStorage().GetGameSystem().GetBaseGameSystemFactoryFirst()) != NULL;

	if(bResult)
	{
		g_pGSFactoryCSpawnGroupMgrGameSystem = reinterpret_cast<decltype(g_pGSFactoryCSpawnGroupMgrGameSystem)>(CBaseGameSystemFactory::GetFactoryByName("SpawnGroupManagerGameSystem"));
	}

	return bResult;
}

bool EntityManager::ProviderAgent::NotifyEntitySystemUpdated()
{
	return (g_pEntitySystem = (CEntitySystem *)(g_pGameEntitySystem = *(CGameEntitySystem **)((uintptr_t)g_pGameResourceServiceServer + g_pEntityManagerProvider->GetGameDataStorage().GetGameResource().GetEntitySystemOffset()))) != NULL;
}

bool EntityManager::ProviderAgent::NotifyGameEventsUpdated()
{
	return (gameeventmanager = *(IGameEventManager2 **)g_pEntityManagerProvider->GetGameDataStorage().GetSource2Server().GetGameEventManagerPtr()) != NULL;
}

bool EntityManager::ProviderAgent::NotifySpawnGroupMgrUpdated(CSpawnGroupMgrGameSystem *pSpawnGroupManager)
{
	// bool bResult = (g_pSpawnGroupMgr = static_cast<decltype(g_pSpawnGroupMgr)>(g_pGSFactoryCSpawnGroupMgrGameSystem->GetStaticGameSystem())) != NULL;

	// if(bResult)
	// {
	// 	// ...
	// }

	// return bResult;

	auto *pSpawnGroupManagerResult = pSpawnGroupManager ? pSpawnGroupManager : *g_pEntityManagerProvider->GetGameDataStorage().GetSpawnGroup().GetSpawnGroupMgrAddress();

	g_pSpawnGroupMgr = pSpawnGroupManagerResult;
	g_pEntityManagerSpawnGroup->SetManager(pSpawnGroupManagerResult);

	return pSpawnGroupManagerResult != NULL;
}

CGameEntitySystem *EntityManager::ProviderAgent::GetSystem()
{
	NotifyEntitySystemUpdated();

	return g_pGameEntitySystem;
}

bool EntityManager::ProviderAgent::ErectResourceManifest(ISpawnGroup *pSpawnGroup, int nCount, const EntitySpawnInfo_t *pEntities, const matrix3x4a_t *const vWorldOffset)
{
	return m_aResourceManifest.Erect(pSpawnGroup, nCount, pEntities, vWorldOffset);
}

IEntityResourceManifest *EntityManager::ProviderAgent::GetResouceManifest()
{
	return m_aResourceManifest.GetEntityPart();
}

void EntityManager::ProviderAgent::AddResourceToEntityManifest(IEntityResourceManifest *pManifest, const char *pszPath)
{
	g_pEntityManagerProvider->GetGameDataStorage().GetEntityResourceManifest().AddResource(pManifest, pszPath);
}

EntityManager::ProviderAgent::ISpawnGroupInstance *EntityManager::ProviderAgent::CreateSpawnGroup()
{
	CSpawnGroupInstance *pSpawnGroup = new CSpawnGroupInstance();

	if(pSpawnGroup)
	{
		m_vecSpawnGroups.AddToTail(pSpawnGroup);
	}

	return static_cast<IEntityManager::IProviderAgent::ISpawnGroupInstance *>(pSpawnGroup);
}

bool EntityManager::ProviderAgent::ReleaseSpawnGroup(EntityManager::ProviderAgent::ISpawnGroupInstance *pSpawnGroup)
{
	bool bResult = m_vecSpawnGroups.FindAndFastRemove(pSpawnGroup);

	if(bResult)
	{
		delete pSpawnGroup;
	}

	return bResult;
}

void EntityManager::ProviderAgent::ReleaseSpawnGroups()
{
	m_vecSpawnGroups.PurgeAndDeleteElements();
}

void EntityManager::ProviderAgent::OnSpawnGroupAllocated(SpawnGroupHandle_t hSpawnGroup, ISpawnGroup *pSpawnGroup)
{
	const char *pSpawnGroupLocalNameFixup = pSpawnGroup->GetLocalNameFixup();

	if(pSpawnGroupLocalNameFixup && pSpawnGroupLocalNameFixup[0])
	{
		for(int i = 0; i < m_vecSpawnGroups.Count(); i++)
		{
			ISpawnGroupInstance *pSpawnGroupAgent = m_vecSpawnGroups[i];

			const char *pLocalFixupName = pSpawnGroupAgent->GetLocalFixupName();

			if(pLocalFixupName && pLocalFixupName[0] && !V_strcmp(pSpawnGroupLocalNameFixup, pLocalFixupName))
			{
				pSpawnGroupAgent->OnSpawnGroupAllocated(hSpawnGroup, pSpawnGroup);
			}
		}
	}
}

void EntityManager::ProviderAgent::OnSpawnGroupInit(SpawnGroupHandle_t hSpawnGroup, IEntityResourceManifest *pManifest, IEntityPrecacheConfiguration *pConfig, ISpawnGroupPrerequisiteRegistry *pRegistry)
{
	for(auto *pSpawnGroupAgent : m_vecSpawnGroups)
	{
		if(hSpawnGroup == pSpawnGroupAgent->GetSpawnGroupHandle())
		{
			pSpawnGroupAgent->OnSpawnGroupInit(hSpawnGroup, pManifest, pConfig, pRegistry);
		}
	}
}

void EntityManager::ProviderAgent::OnSpawnGroupCreateLoading(SpawnGroupHandle_t hSpawnGroup, CMapSpawnGroup *pMapSpawnGroup, bool bSynchronouslySpawnEntities, bool bConfirmResourcesLoaded, CUtlVector<const CEntityKeyValues *> &vecKeyValues)
{
	const char *pSpawnGroupLocalNameFixup = pMapSpawnGroup->GetLocalNameFixup();

	if(pSpawnGroupLocalNameFixup && pSpawnGroupLocalNameFixup[0])
	{
		for(auto *pSpawnGroupAgent : m_vecSpawnGroups)
		{
			const char *pLocalFixupName = pSpawnGroupAgent->GetLocalFixupName();

			if(pLocalFixupName && pLocalFixupName[0] && !V_strcmp(pSpawnGroupLocalNameFixup, pLocalFixupName))
			{
				pSpawnGroupAgent->OnSpawnGroupCreateLoading(hSpawnGroup, pMapSpawnGroup, bSynchronouslySpawnEntities, bConfirmResourcesLoaded, vecKeyValues);
			}
		}
	}
}

void EntityManager::ProviderAgent::OnSpawnGroupDestroyed(SpawnGroupHandle_t handle)
{
	FOR_EACH_VEC_BACK(m_vecSpawnGroups, i)
	{
		ISpawnGroupInstance *pSpawnGroupAgent = m_vecSpawnGroups[i];

		if(handle == pSpawnGroupAgent->GetSpawnGroupHandle())
		{
			pSpawnGroupAgent->OnSpawnGroupDestroyed(handle);

			delete pSpawnGroupAgent;
			m_vecSpawnGroups.Remove(i);
		}
	}
}

void EntityManager::ProviderAgent::PushSpawnQueueOld(KeyValues *pOldOne, SpawnGroupHandle_t hSpawnGroup)
{
	CEntityKeyValues *pNewKeyValues = new CEntityKeyValues(&m_aEntityAllocator, EKV_ALLOCATOR_EXTERNAL);

	// Parse attributes.
	{
		const char *pszAttrSectionKey = "attributes";

		KeyValues *pAttributeValues = pOldOne->FindKey(pszAttrSectionKey, false);

		if(pAttributeValues)
		{
			FOR_EACH_VALUE(pAttributeValues, pAttrKeyValue)
			{
				pNewKeyValues->SetString(CKV3MemberName::Make(pAttrKeyValue->GetName()), pAttrKeyValue->GetString(), true);
			}

			// pOldOne->RemoveSubKey(pAttributeValues, true, true);
		}
	}

	FOR_EACH_VALUE(pOldOne, pKeyValue)
	{
		pNewKeyValues->SetString(CKV3MemberName::Make(pKeyValue->GetName()), pKeyValue->GetString());
	}

	PushSpawnQueue(pNewKeyValues, hSpawnGroup);
}

void EntityManager::ProviderAgent::PushSpawnQueue(CEntityKeyValues *pKeyValues, SpawnGroupHandle_t hSpawnGroup)
{
	SpawnData aSpawn {pKeyValues, hSpawnGroup};

	pKeyValues->AddRef();
	m_vecEntitySpawnQueue.AddToTail(aSpawn);
}

int EntityManager::ProviderAgent::CopySpawnQueueWithEntitySystemOwnership(CUtlVector<const CEntityKeyValues *> &vecTarget, SpawnGroupHandle_t hSpawnGroup)
{
	const int nOldCount = vecTarget.Count();

	auto *pEntitySystemAllocator = g_pGameEntitySystem->GetEntityKeyValuesAllocator();

	for(const auto &aSpawnEntity : m_vecEntitySpawnQueue)
	{
		if(hSpawnGroup == ANY_SPAWN_GROUP || hSpawnGroup == aSpawnEntity.GetSpawnGroup())
		{
			CEntityKeyValues *pESKeyValues = new CEntityKeyValues(pEntitySystemAllocator, EKV_ALLOCATOR_EXTERNAL);

			pESKeyValues->CopyFrom(aSpawnEntity.GetKeyValues(), false, false);
			g_pGameEntitySystem->AddRefKeyValues(pESKeyValues);
			vecTarget.AddToTail(pESKeyValues);
		}
	}

	return vecTarget.Count() - nOldCount;
}

bool EntityManager::ProviderAgent::HasInSpawnQueue(const CEntityKeyValues *pKeyValues, SpawnGroupHandle_t *pResultHandle)
{
	FOR_EACH_VEC(m_vecEntitySpawnQueue, i)
	{
		const auto &aSpawn = m_vecEntitySpawnQueue[i];

		if(aSpawn.GetKeyValues() == pKeyValues)
		{
			if(pResultHandle)
			{
				*pResultHandle = aSpawn.GetSpawnGroup();
			}

			return true;
		}
	}

	return false;
}

bool EntityManager::ProviderAgent::HasInSpawnQueue(SpawnGroupHandle_t hSpawnGroup)
{
	FOR_EACH_VEC(m_vecEntitySpawnQueue, i)
	{
		const auto &aSpawn = m_vecEntitySpawnQueue[i];

		if(aSpawn.GetSpawnGroup() == hSpawnGroup)
		{
			return true;
		}
	}

	return false;
}

int EntityManager::ProviderAgent::ReleaseSpawnQueued(SpawnGroupHandle_t hSpawnGroup)
{
	auto &vecEntitySpawnQueue = m_vecEntitySpawnQueue;

	const int nOldCount = vecEntitySpawnQueue.Count();

	// Find and fast remove.
	{
		if(hSpawnGroup == ANY_SPAWN_GROUP)
		{
			vecEntitySpawnQueue.Purge();
		}
		else
		{
			FOR_EACH_VEC_BACK(vecEntitySpawnQueue, i)
			{
				auto &aSpawn = vecEntitySpawnQueue[i];

				if(hSpawnGroup == aSpawn.GetSpawnGroup())
				{
					vecEntitySpawnQueue.FastRemove(i);
				}
			}
		}
	}

	return vecEntitySpawnQueue.Count() - nOldCount;
}

int EntityManager::ProviderAgent::ExecuteSpawnQueued(SpawnGroupHandle_t hSpawnGroup, CUtlVector<CEntityInstance *> *pEntities, IEntityListener *pListener, CUtlVector<CUtlString> *pDetails, CUtlVector<CUtlString> *pWarnings)
{
	int nExecutedQueueLength = 0;

	CEntitySystemProvider *pEntitySystem = (CEntitySystemProvider *)g_pGameEntitySystem;

	const CEntityIndex iForceEdictIndex = CEntityIndex(-1);

	const EntityKeyId_t aClassnameKey {"classname"};

	CBufferStringGrowable<256> sBuffer;

	for(const auto &aSpawn : m_vecEntitySpawnQueue)
	{
		if(hSpawnGroup == ANY_SPAWN_GROUP || hSpawnGroup == aSpawn.GetSpawnGroup())
		{
			const auto *pKeyValues = aSpawn.GetKeyValues();

			if(pKeyValues)
			{
				const char *pszClassname = pKeyValues->GetString(aClassnameKey);

				if(pszClassname && pszClassname[0])
				{
					CEntityInstance *pEntity = pEntitySystem->CreateEntity(aSpawn.GetSpawnGroup(), pszClassname, ENTITY_NETWORKING_MODE_DEFAULT, iForceEdictIndex, -1, false);

					if(pEntities)
					{
						pEntities->AddToTail(pEntity);
					}

					if(pListener)
					{
						pListener->OnEntityCreated(pEntity, pKeyValues);
					}

					if(pEntity)
					{
						if(pDetails)
						{
							sBuffer.Format("Created \"%s\" (force edict index is %d, result index is %d) entity", pszClassname, iForceEdictIndex.Get(), pEntity->m_pEntity->m_EHandle.GetEntryIndex());
							pDetails->AddToTail(sBuffer);
						}

						pEntitySystem->QueueSpawnEntity(pEntity->m_pEntity, pKeyValues);
						nExecutedQueueLength++;
					}
					else if(pWarnings)
					{
						sBuffer.Format("Failed to create \"%s\" entity", pszClassname);
						pWarnings->AddToTail(sBuffer);
					}
				}
				else if(pWarnings)
				{
					sBuffer.Format("Empty entity \"%s\" key", pszClassname);
					pWarnings->AddToTail(sBuffer);
				}
			}
			else if(pWarnings)
			{
				sBuffer.Format("Empty entity \"%s\" key", aClassnameKey.GetString());
				pWarnings->AddToTail(sBuffer);
			}
		}
	}

	if(!pWarnings || pWarnings->Count() < m_vecEntitySpawnQueue.Count())
	{
		pEntitySystem->ExecuteQueuedCreation();
	}

	ReleaseSpawnQueued(hSpawnGroup);

	return nExecutedQueueLength;
}

void EntityManager::ProviderAgent::PushDestroyQueue(CEntityInstance *pEntity)
{
	m_vecEntityDestroyQueue.AddToTail(pEntity);
}

void EntityManager::ProviderAgent::PushDestroyQueue(CEntityIdentity *pEntity)
{
	m_vecEntityDestroyQueue.AddToTail(pEntity->m_pInstance);
}

int EntityManager::ProviderAgent::AddDestroyQueueToTail(CUtlVector<const CEntityIdentity *> &vecTarget)
{
	const auto &vecEntityDestroyQueue = m_vecEntityDestroyQueue;

	const int iQueueLength = vecEntityDestroyQueue.Count();

	CEntityIdentity **pEntityArr = (CEntityIdentity **)malloc(iQueueLength * sizeof(CEntityIdentity *));
	CEntityIdentity **pEntityCur = pEntityArr;

	for(int i = 0; i < iQueueLength; i++)
	{
		const auto &aDestroy = vecEntityDestroyQueue[i];

		*pEntityArr = aDestroy.GetIdnetity();
		pEntityCur++;
	}

	int iResult = vecTarget.AddMultipleToTail(((uintptr_t)pEntityCur - (uintptr_t)pEntityArr) / sizeof(decltype(pEntityArr)), pEntityArr);

	free(pEntityArr);

	return iResult;
}

void EntityManager::ProviderAgent::ReleaseDestroyQueued()
{
	m_vecEntityDestroyQueue.Purge();
}

int EntityManager::ProviderAgent::ExecuteDestroyQueued()
{
	CEntitySystemProvider *pEntitySystem = (CEntitySystemProvider *)g_pGameEntitySystem;

	auto &vecEntityDestroyQueue = m_vecEntityDestroyQueue;

	const int iQueueLength = vecEntityDestroyQueue.Count();

	if(iQueueLength)
	{
		int i = 0;

		do
		{
			pEntitySystem->QueueDestroyEntity(vecEntityDestroyQueue[i].GetIdnetity());
			i++;
		}
		while(i < iQueueLength);

		pEntitySystem->ExecuteQueuedDeletion();
		vecEntityDestroyQueue.Purge();
	}

	return iQueueLength;
}

bool EntityManager::ProviderAgent::DumpOldKeyValues(KeyValues *pOldOne, Logger::Scope &aOutput, Logger::Scope *paWarnings)
{
	FOR_EACH_VALUE(pOldOne, pKeyValue)
	{
		const char *pszName = pKeyValue->GetName(), 
		           *pszValue = pKeyValue->GetString();

		if(V_stristr(pszName, "color"))
		{
			Color rgba = pKeyValue->GetColor();

			ProviderAgent::MakeDumpColorAlpha(rgba);
			aOutput.PushFormat(rgba, "\"%s\" \"%s\"", pszName, pszValue);
		}
		else
		{
			aOutput.PushFormat("\"%s\" \"%s\"", pszName, pszValue);
		}
	}

	return true;
}

bool EntityManager::ProviderAgent::DumpEntityKeyValues(const CEntityKeyValues *pKeyValues, DumpEntityKeyValuesFlags_t eFlags, Logger::Scope &aOutput, Logger::Scope *paWarnings)
{
	bool bResult = pKeyValues != nullptr;

	if(bResult)
	{
		const KeyValues3 *pRoot = *(const KeyValues3 **)((uintptr_t)pKeyValues + /* offsetof(CEntityKeyValues, m_pKeyValues) */ 2 * sizeof(void *));

		bResult = pRoot != nullptr;

		if(bResult)
		{
			for(int i = 0, iMemberCount = pRoot->GetMemberCount(); i < iMemberCount; i++)
			{ 
				const char *pszName = pRoot->GetMemberName(i);

				KeyValues3 *pMember = const_cast<KeyValues3 *>(pRoot->GetMember(i));

				if(pMember)
				{
					char sValue[512];

					int iStoredLength = This::DumpEntityKeyValue(pMember, sValue, sizeof(sValue));

					if(iStoredLength)
					{
						if(eFlags)
						{
							if(eFlags & This::DEKVF_TYPE)
							{
								V_snprintf(&sValue[iStoredLength], sizeof(sValue) - iStoredLength, " // Type is #%d", pMember->GetTypeEx());
							}

							if(eFlags & This::DEKVF_SUBTYPE)
							{
								V_snprintf(&sValue[iStoredLength], sizeof(sValue) - iStoredLength, " // SubType is #%d", pMember->GetSubType());
							}
						}

						if(V_stristr(pszName, "color"))
						{
							Color rgba = pMember->GetColor();

							MakeDumpColorAlpha(rgba);
							aOutput.PushFormat(rgba, "%s = %s", pszName, sValue);
						}
						else
						{
							aOutput.PushFormat("%s = %s", pszName, sValue);
						}
					}
					else
					{
						aOutput.PushFormat("// \"%s\" is empty", pszName);
					}
				}
				else if(paWarnings)
				{
					paWarnings->PushFormat("Failed to get \"%s\" key member", pszName);
				}
			}

			const KeyValues3 *pAttributes = *(const KeyValues3 **)((uintptr_t)pKeyValues + /* offsetof(CEntityKeyValues, m_pAttibutes) */ 3 * sizeof(void *));

			if(pAttributes)
			{
				const int iMemberCount = pAttributes->GetMemberCount();
				
				if(iMemberCount)
				{
					aOutput.PushFormat("%s = ", "attributes");
					aOutput.Push("{");

					for(int i = 0; i < iMemberCount; i++)
					{ 
						const char *pszAttrName = pAttributes->GetMemberName(i);

						KeyValues3 *pMember = const_cast<KeyValues3 *>(pAttributes->GetMember(i));

						if(pMember)
						{
							char sValue[512];

							int iStoredLength = This::DumpEntityKeyValue(pMember, sValue, sizeof(sValue));

							if(iStoredLength)
							{
								V_snprintf(&sValue[iStoredLength], sizeof(sValue) - iStoredLength, " // Type is #%d | SubType is #%d", pMember->GetTypeEx(), pMember->GetSubType());

								if(V_stristr(pszAttrName, "color"))
								{
									Color rgba = pMember->GetColor();

									MakeDumpColorAlpha(rgba);
									aOutput.PushFormat(rgba, "\t%s = %s", pszAttrName, sValue);
								}
								else
								{
									aOutput.PushFormat("\t%s = %s", pszAttrName, sValue);
								}
							}
							else
							{
								aOutput.PushFormat("\t// \"%s\" attribute is empty", pszAttrName);
							}
						}
						else if(paWarnings)
						{
							paWarnings->PushFormat("Failed to get \"%s\" key member of attribute", pszAttrName);
						}
					}

					aOutput.Push("}");
				}
			}
		}
	}
	else if(paWarnings)
	{
		paWarnings->Push("Skip an entity without key values");
	}

	return bResult;
}

int EntityManager::ProviderAgent::DumpEntityKeyValue(KeyValues3 *pMember, char *psBuffer, size_t nMaxLength)
{
	const char *pDest = "unknown";

	switch(pMember->GetTypeEx())
	{
		case KV3_TYPEEX_NULL:
		{
			const char sNullable[] = "null";

			size_t nStoreSize = sizeof(sNullable);

			V_strncpy(psBuffer, (const char *)sNullable, nMaxLength);

			return nMaxLength < nStoreSize ? nMaxLength : nStoreSize;
		}

		case KV3_TYPEEX_BOOL:
		{
			return V_snprintf(psBuffer, nMaxLength, "%s", pMember->GetBool() ? "true" : "false");
		}

		case KV3_TYPEEX_INT:
			switch(pMember->GetSubType())
			{
				case KV3_SUBTYPE_INT8:
					return V_snprintf(psBuffer, nMaxLength, "%hhd", pMember->GetInt8());
				case KV3_SUBTYPE_INT16:
					return V_snprintf(psBuffer, nMaxLength, "%hd", pMember->GetShort());
				case KV3_SUBTYPE_INT32:
					return V_snprintf(psBuffer, nMaxLength, "%d", pMember->GetInt());
				case KV3_SUBTYPE_INT64:
					return V_snprintf(psBuffer, nMaxLength, "%lld", pMember->GetInt64());
				case KV3_SUBTYPE_EHANDLE:
				{
					const CEntityHandle &aHandle = pMember->GetEHandle();

					int iIndex = -1;

					const char *pszClassname = nullptr;

					if(aHandle.IsValid())
					{
						CEntityInstance *pEntity = aHandle.Get();

						if(pEntity)
						{
							iIndex = pEntity->GetEntityIndex().Get();
							pszClassname = pEntity->GetClassname();
						}
						else
						{
							iIndex = aHandle.GetEntryIndex();
						}
					}

					return V_snprintf(psBuffer, nMaxLength, pszClassname && pszClassname[0] ? "\"entity:%d:%s\"" : "\"entity:%d\"", iIndex, pszClassname);
				}
				default:
				{
					// AssertMsg1(0, "KV3: Unrealized int subtype is %d\n", pMember->GetSubType());
					// return 0;
					break;
				}
			}
			break;
		case KV3_TYPEEX_UINT:
			switch(pMember->GetSubType())
			{
				case KV3_SUBTYPE_UINT8:
					return V_snprintf(psBuffer, nMaxLength, "%hhu", pMember->GetUInt8());
				case KV3_SUBTYPE_UINT16:
					return V_snprintf(psBuffer, nMaxLength, "%hu", pMember->GetUShort());
				case KV3_SUBTYPE_UINT32:
					return V_snprintf(psBuffer, nMaxLength, "%u", pMember->GetUInt());
				case KV3_SUBTYPE_UINT64:
					return V_snprintf(psBuffer, nMaxLength, "%llu", pMember->GetUInt64());
				case KV3_SUBTYPE_STRING_TOKEN:
				{
					const CUtlStringToken &aToken = pMember->GetStringToken();

					return V_snprintf(psBuffer, nMaxLength, "\"string_token:0x%x\"", aToken.GetHashCode());
				}
				case KV3_SUBTYPE_EHANDLE:
				{
					const CEntityHandle &aHandle = pMember->GetEHandle();

					int iIndex = -1;

					const char *pszClassname = nullptr;

					if(aHandle.IsValid())
					{
						CEntityInstance *pEntity = aHandle.Get();

						if(pEntity)
						{
							iIndex = pEntity->GetEntityIndex().Get();
							pszClassname = pEntity->GetClassname();
						}
						else
						{
							iIndex = aHandle.GetEntryIndex();
						}
					}

					return V_snprintf(psBuffer, nMaxLength, pszClassname && pszClassname[0] ? "\"entity:%d:%s\"" : "\"entity:%d\"", iIndex, pszClassname);
				}
				default:
				{
					// AssertMsg1(0, "KV3: Unrealized uint subtype is %d\n", pMember->GetSubType());
					// return 0;
					break;
				}
			}
			break;
		case KV3_TYPEEX_DOUBLE:
			switch(pMember->GetSubType())
			{
				case KV3_SUBTYPE_FLOAT32:
					return V_snprintf(psBuffer, nMaxLength, "%f", pMember->GetFloat());
				case KV3_SUBTYPE_FLOAT64:
					return V_snprintf(psBuffer, nMaxLength, "%g", pMember->GetDouble());
				default:
				{
					// AssertMsg1(0, "KV3: Unrealized double subtype is %d\n", pMember->GetSubType());
					// return 0;
					break;
				}
			}
			break;
		case KV3_TYPEEX_STRING:
		case KV3_TYPEEX_STRING_SHORT:
		case KV3_TYPEEX_STRING_EXTERN:
		// case KV3_TYPEEX_BINARY_BLOB:
		// case KV3_TYPEEX_BINARY_BLOB_EXTERN:
			return V_snprintf(psBuffer, nMaxLength, "\"%s\"", pMember->GetString());
		case KV3_TYPEEX_ARRAY:
		case KV3_TYPEEX_ARRAY_FLOAT32:
		case KV3_TYPEEX_ARRAY_FLOAT64:
		case KV3_TYPEEX_ARRAY_INT16:
		case KV3_TYPEEX_ARRAY_INT32:
		case KV3_TYPEEX_ARRAY_UINT8_SHORT:
		case KV3_TYPEEX_ARRAY_INT16_SHORT:
			switch(pMember->GetSubType())
			{
				case KV3_SUBTYPE_ARRAY:
				{
					int iBufCur = 0;

					if(nMaxLength > 2)
					{
						KeyValues3 **pArray = pMember->GetArrayBase();

						if(pArray)
						{
							const int iArrayLength = pMember->GetArrayElementCount();

							if(iArrayLength > 0)
							{
								psBuffer[0] = '[';
								iBufCur = 1;

								int i = 0;

								char *pArrayBuf = (char *)stackalloc(nMaxLength);

								do
								{
									if(ProviderAgent::DumpEntityKeyValue(pArray[i], pArrayBuf, nMaxLength))
									{
										iBufCur += V_snprintf(&psBuffer[iBufCur], nMaxLength - iBufCur, "%s, ", pArrayBuf);
									}

									i++;
								}
								while(i < iArrayLength);

								iBufCur -= 2; // Strip ", " of end.
								psBuffer[iBufCur++] = ']';
								psBuffer[iBufCur] = '\0';
							}
						}
					}

					return iBufCur;
				}

				case KV3_SUBTYPE_VECTOR:
				case KV3_SUBTYPE_ROTATION_VECTOR:
				case KV3_SUBTYPE_QANGLE:
				{
					const Vector &aVec3 = pMember->GetVector();

					return V_snprintf(psBuffer, nMaxLength, "\"%f %f %f\"", aVec3.x, aVec3.y, aVec3.z);
				}
				case KV3_SUBTYPE_VECTOR2D:
				{
					const Vector2D &aVec2 = pMember->GetVector2D();

					return V_snprintf(psBuffer, nMaxLength, "\"%f %f\"", aVec2.x, aVec2.y);
				}
				case KV3_SUBTYPE_VECTOR4D:
				case KV3_SUBTYPE_QUATERNION:
				{
					const Vector4D &aVec4 = pMember->GetVector4D();

					return V_snprintf(psBuffer, nMaxLength, "\"%f %f %f %f\"", aVec4.x, aVec4.y, aVec4.z, aVec4.w);
				}
				case KV3_SUBTYPE_MATRIX3X4:
				{
					const matrix3x4_t &aMatrix3x4 = pMember->GetMatrix3x4();

					return V_snprintf(psBuffer, nMaxLength, "[\"%f %f %f\", \"%f %f %f\", \"%f %f %f\", \"%f %f %f\"]",  
					                                        aMatrix3x4.m_flMatVal[0][0], aMatrix3x4.m_flMatVal[0][1], aMatrix3x4.m_flMatVal[0][2], aMatrix3x4.m_flMatVal[0][3], 
					                                        aMatrix3x4.m_flMatVal[1][0], aMatrix3x4.m_flMatVal[1][1], aMatrix3x4.m_flMatVal[1][2], aMatrix3x4.m_flMatVal[1][3], 
					                                        aMatrix3x4.m_flMatVal[2][0], aMatrix3x4.m_flMatVal[2][1], aMatrix3x4.m_flMatVal[2][2], aMatrix3x4.m_flMatVal[2][3]);
				}
				default:
				{
					// AssertMsg1(0, "KV3: Unrealized array subtype is %d", pMember->GetSubType());
					return 0;
				}
			}
			break;
		default:
			AssertMsg1(0, "KV3: Unrealized typeex is %d", pMember->GetTypeEx());
			return 0;
	}

	return 0;
}


bool EntityManager::ProviderAgent::MakeDumpColorAlpha(Color &rgba)
{
	bool bResult = !rgba[3];

	if(bResult)
	{
		rgba[3] = 127 + (127 * ((rgba[0] + rgba[1] + rgba[2]) / (255 * 3)));
	}

	return bResult;
}

EntityManager::ProviderAgent::SpawnData::SpawnData(CEntityKeyValues *pKeyValues)
 :  SpawnData(pKeyValues, ANY_SPAWN_GROUP)
{
}

EntityManager::ProviderAgent::SpawnData::SpawnData(CEntityKeyValues *pKeyValues, SpawnGroupHandle_t hSpawnGroup)
 :  m_pKeyValues(pKeyValues),
    m_hSpawnGroup(hSpawnGroup)
{
	pKeyValues->AddRef();
}

EntityManager::ProviderAgent::SpawnData::~SpawnData()
{
	Release();
}

const CEntityKeyValues *EntityManager::ProviderAgent::SpawnData::GetKeyValues() const
{
	return m_pKeyValues;
}

SpawnGroupHandle_t EntityManager::ProviderAgent::SpawnData::GetSpawnGroup() const
{
	return m_hSpawnGroup;
}

bool EntityManager::ProviderAgent::SpawnData::IsAnySpawnGroup() const
{
	return GetSpawnGroup() == ANY_SPAWN_GROUP;
}

void EntityManager::ProviderAgent::SpawnData::Release()
{
	m_pKeyValues->Release();
}

EntityManager::ProviderAgent::DestoryData::DestoryData(CEntityInstance *pInstance)
{
	m_pIdentity = pInstance->m_pEntity;
}

EntityManager::ProviderAgent::DestoryData::DestoryData(CEntityIdentity *pIdentity)
{
	m_pIdentity = pIdentity;
}

EntityManager::ProviderAgent::DestoryData::~DestoryData()
{
	// Empty.
}

CEntityIdentity *EntityManager::ProviderAgent::DestoryData::GetIdnetity() const
{
	return m_pIdentity;
}
