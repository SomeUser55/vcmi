﻿/*
 * CObjectClassesHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ObjectTemplate.h"

#include "../GameConstants.h"
#include "../ConstTransitivePtr.h"
#include "../IHandlerBase.h"
#include "../JsonNode.h"

class JsonNode;
class CRandomGenerator;

/// Structure that describes placement rules for this object in random map
struct DLL_LINKAGE RandomMapInfo
{
	/// How valuable this object is, 1k = worthless, 10k = Utopia-level
	ui32 value;

	/// How many of such objects can be placed on map, 0 = object can not be placed by RMG
	ui32 mapLimit;

	/// How many of such objects can be placed in one zone, 0 = unplaceable
	ui32 zoneLimit;

	/// Rarity of object, 5 = extremely rare, 100 = common
	ui32 rarity;

	RandomMapInfo():
		value(0),
		mapLimit(0),
		zoneLimit(0),
		rarity(0)
	{}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & value & mapLimit & zoneLimit & rarity;
	}
};

class DLL_LINKAGE IObjectInfo
{
public:
	struct CArmyStructure
	{
		ui32 totalStrength;
		ui32 shootersStrength;
		ui32 flyersStrength;
		ui32 walkersStrength;

		CArmyStructure() :
			totalStrength(0),
			shootersStrength(0),
			flyersStrength(0),
			walkersStrength(0)
		{}

		bool operator <(const CArmyStructure & other) const
		{
			return this->totalStrength < other.totalStrength;
		}
	};

	/// Returns possible composition of guards. Actual guards would be
	/// somewhere between these two values
	virtual CArmyStructure minGuards() const { return CArmyStructure(); }
	virtual CArmyStructure maxGuards() const { return CArmyStructure(); }

	virtual bool givesResources() const { return false; }

	virtual bool givesExperience() const { return false; }
	virtual bool givesMana() const { return false; }
	virtual bool givesMovement() const { return false; }

	virtual bool givesPrimarySkills() const { return false; }
	virtual bool givesSecondarySkills() const { return false; }

	virtual bool givesArtifacts() const { return false; }
	virtual bool givesCreatures() const { return false; }
	virtual bool givesSpells() const { return false; }

	virtual bool givesBonuses() const { return false; }

	virtual ~IObjectInfo() = default;
};

class CGObjectInstance;

class DLL_LINKAGE AObjectTypeHandler : public boost::noncopyable
{
	RandomMapInfo rmgInfo;

	/// Human-readable name of this object, used for objects like banks and dwellings, if set
	boost::optional<std::string> objectName;

	JsonNode base; /// describes base template

	std::vector<ObjectTemplate> templates;
protected:
	void preInitObject(CGObjectInstance * obj) const;
	virtual bool objectFilter(const CGObjectInstance *, const ObjectTemplate &) const;

	/// initialization for classes that inherit this one
	virtual void initTypeData(const JsonNode & input);
public:
	std::string typeName;
	std::string subTypeName;

	si32 type;
	si32 subtype;
	AObjectTypeHandler();
	virtual ~AObjectTypeHandler();

	void setType(si32 type, si32 subtype);
	void setTypeName(std::string type, std::string subtype);

	/// loads generic data from Json structure and passes it towards type-specific constructors
	void init(const JsonNode & input, boost::optional<std::string> name = boost::optional<std::string>());

	/// Returns object-specific name, if set
	boost::optional<std::string> getCustomName() const;

	void addTemplate(const ObjectTemplate & templ);
	void addTemplate(JsonNode config);

	/// returns all templates matching parameters
	std::vector<ObjectTemplate> getTemplates() const;
	std::vector<ObjectTemplate> getTemplates(si32 terrainType) const;

	/// returns preferred template for this object, if present (e.g. one of 3 possible templates for town - village, fort and castle)
	/// note that appearance will not be changed - this must be done separately (either by assignment or via pack from server)
	boost::optional<ObjectTemplate> getOverride(si32 terrainType, const CGObjectInstance * object) const;

	const RandomMapInfo & getRMGInfo();

	virtual bool isStaticObject();

	virtual void afterLoadFinalization();

	/// Creates object and set up core properties (like ID/subID). Object is NOT initialized
	/// to allow creating objects before game start (e.g. map loading)
	virtual CGObjectInstance * create(const ObjectTemplate & tmpl) const = 0;

	/// Configures object properties. Should be re-entrable, resetting state of the object if necessarily
	/// This should set remaining properties, including randomized or depending on map
	virtual void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const = 0;

	/// Returns object configuration, if available. Otherwise returns NULL
	virtual std::unique_ptr<IObjectInfo> getObjectInfo(const ObjectTemplate & tmpl) const = 0;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type & subtype & templates & rmgInfo & objectName;
		if(version >= 759)
		{
			h & typeName & subTypeName;
		}
	}
};

typedef std::shared_ptr<AObjectTypeHandler> TObjectTypeHandler;

class DLL_LINKAGE CObjectClassesHandler : public IHandlerBase
{
	/// Small internal structure that contains information on specific group of objects
	/// (creating separate entity is overcomplicating at least at this point)
	struct ObjectContainter
	{
		si32 id;
		std::string identifier;
		std::string name; // human-readable name
		std::string handlerName; // ID of handler that controls this object, should be determined using handlerConstructor map

		JsonNode base;
		std::map<si32, TObjectTypeHandler> subObjects;
		std::map<std::string, si32> subIds;//full id from core scope -> subtype

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & name & handlerName & base & subObjects;
			if(version >= 759)
			{
				h & identifier & subIds;
			}
		}
	};

	/// list of object handlers, each of them handles only one type
	std::map<si32, ObjectContainter * > objects;

	/// map that is filled during contruction with all known handlers. Not serializeable due to usage of std::function
	std::map<std::string, std::function<TObjectTypeHandler()> > handlerConstructors;

	/// container with H3 templates, used only during loading, no need to serialize it
	typedef std::multimap<std::pair<si32, si32>, ObjectTemplate> TTemplatesContainer;
	TTemplatesContainer legacyTemplates;

	/// contains list of custom names for H3 objects (e.g. Dwellings), used to load H3 data
	/// format: customNames[primaryID][secondaryID] -> name
	std::map<si32, std::vector<std::string>> customNames;

	void loadObjectEntry(const std::string & identifier, const JsonNode & entry, ObjectContainter * obj);
	ObjectContainter * loadFromJson(const JsonNode & json, const std::string & name);
public:
	CObjectClassesHandler();
	~CObjectClassesHandler();

	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	void loadSubObject(const std::string & identifier, JsonNode config, si32 ID, boost::optional<si32> subID = boost::optional<si32>());
	void removeSubObject(si32 ID, si32 subID);

	void beforeValidate(JsonNode & object) override;
	void afterLoadFinalization() override;

	std::vector<bool> getDefaultAllowed() const override;

	/// Queries to detect loaded objects
	std::set<si32> knownObjects() const;
	std::set<si32> knownSubObjects(si32 primaryID) const;

	/// returns handler for specified object (ID-based). ObjectHandler keeps ownership
	TObjectTypeHandler getHandlerFor(si32 type, si32 subtype) const;
	TObjectTypeHandler getHandlerFor(std::string type, std::string subtype) const;

	std::string getObjectName(si32 type) const;
	std::string getObjectName(si32 type, si32 subtype) const;

	/// Returns handler string describing the handler (for use in client)
	std::string getObjectHandlerName(si32 type) const;



	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objects;
	}
};
