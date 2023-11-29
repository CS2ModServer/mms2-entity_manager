#ifndef _INCLUDE_METAMOD_SOURCE_ENTITY_MANAGER_PROVIDER_AGENT_
#define _INCLUDE_METAMOD_SOURCE_ENTITY_MANAGER_PROVIDER_AGENT_

#include <stddef.h>

#include <eiface.h>
#include <tier1/utlmap.h>
#include <tier0/utlbinaryblock.h>
#include <tier1/utlvector.h>
#include <entity2/entityidentity.h>

#include "logger.h"
#include "provider/entitykeyvalues.h"
#include "provider_agent/resourcemanifest.h"

#define LOGGER_COLOR_KEYVALUES {241, 160, 50, 255}
#define LOGGER_COLOR_ENTITY_KV3 {0, 191, 255, 255}

#define INVALID_SPAWN_GROUP ((SpawnGroupHandle_t)-1)

class KeyValues;
class CEntityKeyValues;

namespace EntityManager
{
	class ProviderAgent
	{
	public:
		ProviderAgent();

		bool Init(char *psError, size_t nMaxLength);
		void Clear();
		void Destroy();

	public:
		virtual bool NotifyGameResourceUpdated();
		virtual bool NotifyEntitySystemUpdated();
		virtual bool NotifySpawnGroupMgrUpdated();

	public:
		bool ErectResourceManifest(SpawnGroupHandle_t hSpawnGroup, int nCount, const EntitySpawnInfo_t *pEntities, const matrix3x4a_t *const vWorldOffset);
		IEntityResourceManifest *GetMyEntityManifest();

	public: // Spawn queue methods.
		void PushSpawnQueueOld(KeyValues *pOldOne, SpawnGroupHandle_t hSpawnGroup = INVALID_SPAWN_GROUP, Logger::Scope *pWarnings = nullptr);
		void PushSpawnQueue(CEntityKeyValues *pKeyValues, SpawnGroupHandle_t hSpawnGroup = INVALID_SPAWN_GROUP);
		int AddSpawnQueueToTail(CUtlVector<const CEntityKeyValues *> *&pvecTarget, SpawnGroupHandle_t hSpawnGroup = INVALID_SPAWN_GROUP);
		bool HasInSpawnQueue(const CEntityKeyValues *pKeyValues);
		int ReleaseSpawnQueued(SpawnGroupHandle_t hSpawnGroup = INVALID_SPAWN_GROUP);
		int SpawnQueued(SpawnGroupHandle_t hSpawnGroup = INVALID_SPAWN_GROUP, Logger::Scope *pDetails = nullptr, Logger::Scope *pWarnings = nullptr);

	public: // Destroy queue methods.
		void PushDestroyQueue(CEntityInstance *pEntity);
		void PushDestroyQueue(CEntityIdentity *pEntity);
		int AddDestroyQueueToTail(CUtlVector<const CEntityIdentity *> *&pvecTarget);
		void ReleaseDestroyQueued();
		int DestroyQueued();

	public: // Dumps.
		static bool DumpOldKeyValues(KeyValues *pOldOne, Logger::Scope &aOutput, Logger::Scope *paWarnings = nullptr);
		static bool DumpEntityKeyValues(const CEntityKeyValues *pKeyValues, Logger::Scope &aOutput, Logger::Scope *paWarnings = nullptr);
		static int DumpEntityKeyValue(KeyValues3 *pMember, char *psBuffer, size_t nMaxLength);
		static bool MakeDumpColorAlpha(Color &rgba);

	protected: // Cache of entity keys.
		typedef CUtlMap<CUtlBinaryBlock, EntityKeyId_t> CacheMapType;
		typedef CacheMapType::IndexType_t CacheMapOIndexType;

		const EntityKeyId_t &GetCachedEntityKey(CacheMapOIndexType nElm);
		const EntityKeyId_t &GetCachedClassnameKey();
		CacheMapOIndexType CacheOrGetEntityKey(const char *pszName);

	private:
		class SpawnData
		{
			typedef SpawnData ThisClass;

		public:
			SpawnData(CEntityKeyValues *pKeyValues);
			SpawnData(CEntityKeyValues *pKeyValues, SpawnGroupHandle_t hSpawnGroup);
			~SpawnData();

			CEntityKeyValues *GetKeyValues() const;
			CEntityKeyValuesProvider *GetKeyValuesProvider() const;

			SpawnGroupHandle_t GetSpawnGroup() const;
			static SpawnGroupHandle_t GetAnySpawnGroup();
			bool IsAnySpawnGroup() const;

			void Release();

		private:
			CEntityKeyValues *m_pKeyValues;
			SpawnGroupHandle_t m_hSpawnGroup;
		};

		CUtlVector<SpawnData> m_vecEntitySpawnQueue;

		struct DestoryData
		{
			typedef DestoryData ThisClass;

		public:
			DestoryData(CEntityInstance *pInstance);
			DestoryData(CEntityIdentity *pIdentity);
			~DestoryData();

			CEntityIdentity *GetIdnetity() const;

		private:
			CEntityIdentity *m_pIdentity;
		};

		CUtlVector<DestoryData> m_vecEntityDestroyQueue;

		CacheMapType m_mapCachedKeys;
		CacheMapOIndexType m_nElmCachedClassnameKey;

		ResourceManifest m_aResourceManifest;
	};
};

#endif //_INCLUDE_METAMOD_SOURCE_ENTITY_MANAGER_PROVIDER_AGENT_
