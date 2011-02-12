#ifndef __CDEFOBJINFOHANDLER_H__
#define __CDEFOBJINFOHANDLER_H__
#include <vector>
#include <map>
#include "../global.h"
#include "../lib/ConstTransitivePtr.h"

/*
 * CDefObjInfoHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CDefEssential;
class CLodHandler;
class DLL_EXPORT CGDefInfo
{
public:
	std::string name;

	ui8 visitMap[6];
	ui8 blockMap[6];
	ui8 coverageMap[6], shadowCoverage[6]; //to determine which tiles are covered by picture of this object
	ui8 visitDir; //directions from which object can be entered, format same as for moveDir in CGHeroInstance(but 0 - 7)
	si32 id, subid; //of object described by this defInfo
	si32 terrainAllowed, //on which terrain it is possible to place object
		 terrainMenu; //in which menus in map editor object will be showed
	CDefEssential * handler; 
	si32 width, height; //tiles
	si32 type; //(0- ground, 1- towns, 2-creatures, 3- heroes, 4-artifacts, 5- resources)   
	si32 printPriority;
	bool isVisitable() const;
	bool operator<(const CGDefInfo& por) const
	{
		if(id!=por.id)
			return id<por.id;
		else
			return subid<por.subid;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & visitMap & blockMap & visitDir & id & subid &terrainAllowed
			& terrainMenu & width & height & type & printPriority & coverageMap & shadowCoverage;
	}
	CGDefInfo();
void fetchInfoFromMSK();
};
class DLL_EXPORT CDefObjInfoHandler
{
public:
	bmap<int, bmap<int, ConstTransitivePtr<CGDefInfo> > > gobjs;
	bmap<int, ConstTransitivePtr<CGDefInfo> > castles;

	void load();
	~CDefObjInfoHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & gobjs;
		h & castles;
	}
};

#endif // __CDEFOBJINFOHANDLER_H__