/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod:Source Entity Manager
 * Written by Wend4r.
 * ======================================================

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef _INCLUDE_METAMOD_SOURCE_IENTITYMGR_HPP_
#	define _INCLUDE_METAMOD_SOURCE_IENTITYMGR_HPP_

#	pragma once

#	include <stddef.h>

#	include <functional>

#	include <tier1/utlvector.h>
#	include <entity2/entityidentity.h>

#	define ENTITY_MANAGER_INTERFACE_NAME "Entity Manager v1.0"

#	define INVALID_SPAWN_GROUP ((SpawnGroupHandle_t)-1)
#	define ANY_SPAWN_GROUP INVALID_SPAWN_GROUP

class IEntityResourceManifest;
struct EntitySpawnInfo_t;
class KeyValues;
class CEntityKeyValues;

class ISpawnGroup;
struct SpawnGroupDesc_t;
class Vector;

/**
 * @brief A Entity Manager interface.
 * Note: gets with "ismm->MetaFactory(ENTITY_MANAGER_INTERFACE_NAME, NULL, NULL);"
**/
class IEntityManager
{
public: // Provider agent ones.
	/**
	 * @brief A provider agent interface.
	 */
	class IProviderAgent
	{
	public: // Spawn group ones.
		/**
		 * @brief A spawn group loader interface.
		 */
		class ISpawnGroupLoader
		{
		public:
			/**
			 * @brief Load the spawn group by a description.
			 * 
			 * @param aDesc             A spawn group description.
			 * @param vecLandmarkOffset A landmark offset.
			 * 
			 * @return                  True if successed added to the queue, 
			 *                          otherwise false if failed to add.
			 */
			virtual bool Load(const SpawnGroupDesc_t &aDesc, const Vector &vecLandmarkOffset) = 0;

			/**
			 * @brief Unload the spawn group.
			 * 
			 * @return                  True if successed added to the queue, 
			 *                          otherwise false if failed to add.
			 */
			virtual bool Unload() = 0;
		}; // ISpawnGroupLoader

		/**
		 * @brief A spawn group notifications interface.
		 */
		class ISpawnGroupNotifications
		{
		public:
			/**
			 * @brief Calls when spawn group are allocated.
			 * 
			 * @param handle            A spawn group handle to destroy.
			 * @param pSpawnGroup       A spawn group pointer.
			 */
			virtual void OnSpawnGroupAllocated(SpawnGroupHandle_t handle, ISpawnGroup *pSpawnGroup) = 0;

			/**
			 * @brief Calls when spawn group are destroyed.
			 * 
			 * @param handle            A spawn group handle to destroy.
			 */
			virtual void OnSpawnGroupDestroyed(SpawnGroupHandle_t handle) = 0;
		}; // ISpawnGroupNotifications

		/**
		 * @brief A spawn group instance interface.
		**/
		class ISpawnGroupInstance : public ISpawnGroupLoader, public ISpawnGroupNotifications
		{
		public:
			/**
			 * @brief Destructor of the instance, which calls the unload of the spawn group. 
			 *        Used internally, call `ReleaseSpawnGroup` instead.
			 */
			virtual ~ISpawnGroupInstance() {};

			/**
			 * @brief Gets the status of the spawn group.
			 * 
			 * @return                  The status value of a spawn group.
			 */
			virtual int GetStatus() const = 0;

			/**
			 * @brief Gets a spawn group.
			 * 
			 * @return                  A spawn group pointer.
			 */
			virtual ISpawnGroup *GetSpawnGroup() const = 0;

			/**
			 * @brief Gets an allocated spawn group handle.
			 * 
			 * @return                  A spawn group handle.
			 */
			virtual SpawnGroupHandle_t GetSpawnGroupHandle() const = 0;

			/**
			 * @brief Gets a level name string.
			 * 
			 * @return                  A string of the name.
			 */
			virtual const char *GetLevelName() const = 0;

			/**
			 * @brief Gets a landmark name string.
			 * 
			 * @return                  A string of the landmark name.
			 */
			virtual const char *GetLandmarkName() const = 0;

			/**
			 * @brief Gets a landmark offset.
			 * 
			 * @return                  A vector of the offset.
			 */
			virtual const Vector &GetLandmarkOffset() const = 0;
		}; // ISpawnGroupInstance

	public: // A entity system things.
		/**
		 * @brief Allocates a pooled string of the entity system.
		 * 
		 * @param pString           A string to allocate.
		 */
		virtual CUtlSymbolLarge AllocPooledString(const char *pString) = 0;

		/**
		 * @brief Find a pooled string in the entity system.
		 * 
		 * @param pString           A string to find.
		 */
		virtual CUtlSymbolLarge FindPooledString(const char* pString) = 0;

	public:
		/**
		 * @brief Erect a resourse manifest with a spawn group.
		 * 
		 * @param pSpawnGroup       A spawn group.
		 * @param nCount            A count of entities.
		 * @param pEntities         A spawn infos of entities.
		 * @param vWorldOffset      A world offset.
		 */
		virtual bool ErectResourceManifest(ISpawnGroup *pSpawnGroup, int nCount, const EntitySpawnInfo_t *pEntities, const matrix3x4a_t *const vWorldOffset) = 0;

		/**
		 * @brief Gets an entity manifest.
		 * 
		 * @return                  An entity manifest pointer.
		 */
		virtual IEntityResourceManifest *GetEntityManifest() = 0;

		/**
		 * @brief Creates a spawn group instance. 
		 *        To load, use the `Load` method
		 * 
		 * @return                  An entity manifest pointer.
		 */
		virtual ISpawnGroupInstance *CreateSpawnGroup() = 0;

		/**
		 * @brief Releases a spawn group instance. 
		 *        To load, use the `Load` method
		 * 
		 * @return                  True if spawn group are released, 
		 *                          otherwise false if failed to find.
		 */
		virtual bool ReleaseSpawnGroup(ISpawnGroupInstance *pSpawnGroupInstance) = 0;

		/**
		 * @brief Push an old entity structure to the spawn queue.
		 * 
		 * @param pOldOne           An old entity structure.
		 * @param hSpawnGroup       A spawn group on which the entity should spawned.
		 *                          If ANY_SPAWN_GROUP, entity will spawned on the active spawn group.
		 */
		virtual void PushSpawnQueueOld(KeyValues *pOldOne, SpawnGroupHandle_t hSpawnGroup = ANY_SPAWN_GROUP) = 0;

		/**
		 * @brief Push an entity structure to the spawn queue.
		 * 
		 * @param pKeyValues        An entity structure.
		 * @param hSpawnGroup       A spawn group on which the entity should spawned.
		 *                          If ANY_SPAWN_GROUP, entity will spawned on the active spawn group.
		 */
		virtual void PushSpawnQueue(CEntityKeyValues *pKeyValues, SpawnGroupHandle_t hSpawnGroup = ANY_SPAWN_GROUP) = 0;

		/**
		 * @brief Adds the spawn queue to a vector.
		 * 
		 * @param vecTarget         A vector target.
		 * @param hSpawnGroup       A filter spawn group.
		 *                          If ANY_SPAWN_GROUP, will be added all entities.
		 * 
		 * @return                  Return the number of added elements.
		 */
		virtual int AddSpawnQueueToTail(CUtlVector<const CEntityKeyValues *> &vecTarget, SpawnGroupHandle_t hSpawnGroup = ANY_SPAWN_GROUP) = 0;

		/**
		 * @brief Checks to has entity structure in the spawn queue.
		 * 
		 * @param pKeyValues        An entity structure to find.
		 * @param pResultHandle     An optional spawn group handle pointer of the found entity.
		 * 
		 * @return                  True if found the the first entity by the spawn group handle, 
		 *                          otherwise false if not found.
		 */
		virtual bool HasInSpawnQueue(const CEntityKeyValues *pKeyValues, SpawnGroupHandle_t *pResultHandle = nullptr) = 0;

		/**
		 * @brief Checks to has spawn group in the spawn queue.
		 * 
		 * @param hSpawnGroup       A spawn group handle to find.
		 * 
		 * @return                  True if found the the first entity by the spawn group handle, 
		 *                          otherwise false if not found.
		 */
		virtual bool HasInSpawnQueue(SpawnGroupHandle_t hSpawnGroup = ANY_SPAWN_GROUP) = 0;

		/**
		 * @brief Releases/Destroy spawn queued entities.
		 * 
		 * @param hSpawnGroup       A spawn group filter.
		 * 
		 * @return                  Released count of the queued entities.
		 */
		virtual int ReleaseSpawnQueued(SpawnGroupHandle_t hSpawnGroup = ANY_SPAWN_GROUP) = 0;

		/**
		 * @brief Executes the spawn queued entities.
		 * 
		 * @param hSpawnGroup       A spawn group to spawn.
		 * @param pEntities         An optional vector pointer of created entities.
		 * @param pDetails          An optional vector pointer of detailed messages.
		 * @param pWarnings         An opitonal vector pointer of warning messages.
		 * 
		 * @return                  The executed spawn queue length.
		 */
		virtual int ExecuteSpawnQueued(SpawnGroupHandle_t hSpawnGroup = ANY_SPAWN_GROUP, CUtlVector<CEntityInstance *> *pEntities = nullptr, CUtlVector<CUtlString> *pDetails = nullptr, CUtlVector<CUtlString> *pWarnings = nullptr) = 0;
	};

	/**
	 * @brief Gets a provider agent.
	 * 
	 * @return                  A provider agent pointer.
	 */
	virtual IProviderAgent *GetProviderAgent() = 0;
}; // IEntityManager

#endif // _INCLUDE_METAMOD_SOURCE_IENTITYMGR_HPP_
