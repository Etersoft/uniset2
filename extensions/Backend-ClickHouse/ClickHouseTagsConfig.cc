#include <sstream>
#include <iomanip>

#include "Exceptions.h"
#include "UniXML.h"
#include "UniSetTypes.h"
#include "ClickHouseTagsConfig.h"
// ------------------------------------------------------------------------------------------
using namespace std;
using namespace uniset;
// ------------------------------------------------------------------------------------------
ClickHouseTagsConfig::ClickHouseTagsConfig()
{
}
//--------------------------------------------------------------------------------------------
ClickHouseTagsConfig::~ClickHouseTagsConfig()
{

}
//--------------------------------------------------------------------------------------------
void uniset::ClickHouseTagsConfig::load( const std::shared_ptr<uniset::Configuration> conf
										 , const std::string ff
										 , const std::string fv)
{
	auto xml = conf->getConfXML();

	// init tags map
	UniXML::iterator tit = xml->findNode(xml->getFirstNode(),"clickhouse");
	if( !tit )
	{
		ostringstream err;
		err << "(ClickHouseTagsConfig): not found <clickhouse> section in configure " << conf->getConfFileName();
		throw uniset::SystemError(err.str());
	}

	tagsmap = initTagsMap(tit);

	// init tags
	UniXML::iterator it = conf->getXMLSensorsSection();

	if( !it )
	{
		ostringstream err;
		err << "(ClickHouseTagsConfig): not found <sensors> section in configure" << conf->getConfFileName();
		throw uniset::SystemError(err.str());
	}

	if( !it.goChildren() )
	{
		ostringstream err;
		err << "(ClickHouseTagsConfig): <sensors> section is empty?! in configure " << conf->getConfFileName();
		throw uniset::SystemError(err.str());
	}

	for( ;it.getCurrent(); ++it )
	{
		if( !uniset::check_filter(it, ff, fv) )
			continue;

		initFromItem(conf, it);
	}
}
//--------------------------------------------------------------------------------------------
void ClickHouseTagsConfig::loadTagsMap( uniset::UniXML::iterator it )
{
	tagsmap = initTagsMap(it);
}
//--------------------------------------------------------------------------------------------
void ClickHouseTagsConfig::initFromItem( const std::shared_ptr<uniset::Configuration> conf
										 , const uniset::UniXML::iterator& it )
{
	std::string s_tags = it.getProp("clickhouse_dyn_tags");

	// init from 'clickhouse_dyn_s_tags' property
	if( !s_tags.empty() )
	{
		initFromTags(conf, it, s_tags);
		return;
	}

	// init from clickhouse_dyn_values
	std::string s_values = it.getProp("clickhouse_dyn_values");
	if( !s_values.empty() )
	{
		initFromValues(conf, it, s_values);
		return;
	}

	// init from <clickhouse_tags> section
	UniXML::iterator it2 = it;
	if( !it2.goChildren() )
		return;

	if( !it2.find("clickhouse_tags") )
		return;

	if( !it2.goChildren() )
	{
		ostringstream err;
		err << "(ClickHouseTagsConfig): found empty section <clickhouse_tags> for sensor " << it.getProp("name");
		throw uniset::SystemError(err.str());
	}

	initFromTagsList(conf, it, it2);
}
//--------------------------------------------------------------------------------------------
void ClickHouseTagsConfig::initFromTags( const std::shared_ptr<uniset::Configuration>& conf
								   , UniXML::iterator it
								   , const std::string s_tags )
{
	uniset::ObjectId myId = it.getIntProp("id");
	std::string s_id = it.getProp("clickhouse_dyn_sid");
	uniset::ObjectId sid = conf->getSensorID(s_id);
	if( sid == uniset::DefaultObjectId )
	{
		ostringstream err;
		err << "(ClickHouseTagsConfig): not found sensor ID for sensor " << s_id
			<< " in <clickhouse> section for " << it.getProp("name");
		throw uniset::SystemError(err.str());
	}

	long defvalue = it.getIntProp("default");

	auto tagsIt = tags.find(myId);
	if( tagsIt == tags.end() )
	{
		auto ret = tags.emplace(myId,TagList());
		tagsIt = ret.first;
	}

	TagsInfo inf;
	inf.any_tags = parseTags(s_tags,' ');
	inf.it = addState(sid, defvalue);
	inf.sid = sid;
	TagList lst = { inf };
	tagsIt->second.swap(lst);
}
//--------------------------------------------------------------------------------------------
void ClickHouseTagsConfig::initFromValues( const std::shared_ptr<uniset::Configuration>& conf
									 , UniXML::iterator it
									 , const std::string s_values )
{
	uniset::ObjectId myId = it.getIntProp("id");
	std::string s_id = it.getProp("clickhouse_dyn_sid");
	uniset::ObjectId sid = conf->getSensorID(s_id);
	if( sid == uniset::DefaultObjectId )
	{
		ostringstream err;
		err << "(ClickHouseTagsConfig): not found sensor ID for sensor " << s_id
			<< " in <clickhouse> section for " << it.getProp("name");
		throw uniset::SystemError(err.str());
	}

	auto vit = tagsmap.find(s_values);
	if( vit == tagsmap.end() )
	{
		ostringstream err;
		err << "(ClickHouseTagsConfig): not found clickhouse_dyn_sid='" << s_values << "'"
			<< " in section <clickhouse> for " << it.getProp("name");
		throw uniset::SystemError(err.str());
	}

	auto tagsIt = tags.find(myId);
	if( tagsIt == tags.end() )
	{
		auto ret = tags.emplace(myId,TagList());
		tagsIt = ret.first;
	}

	TagsInfo inf;
	inf.vit = vit;
	inf.it = addState(sid, it.getIntProp("default"));
	inf.sid = sid;
	TagList lst = { inf };
	tagsIt->second.swap(lst);
}
//--------------------------------------------------------------------------------------------
void ClickHouseTagsConfig::initFromTagsList( const std::shared_ptr<uniset::Configuration>& conf
									   , uniset::UniXML::iterator it
									   , uniset::UniXML::iterator itList )
{
	uniset::ObjectId myId = it.getIntProp("id");

	for( ; itList.getCurrent(); ++itList )
	{
		uniset::ObjectId sid = conf->getSensorID(itList.getProp("name"));
		if( sid == uniset::DefaultObjectId )
		{
			ostringstream err;
			err << "(ClickHouseTagsConfig): not found sensor ID for sensor " << itList.getProp("name")
				<< " in section <clickhouse> for " << it.getProp("name");
			throw uniset::SystemError(err.str());
		}

		auto vit = tagsmap.find(itList.getProp("values"));
		if( vit == tagsmap.end() )
		{
			ostringstream err;
			err << "(ClickHouseTagsConfig): not found values='" << itList.getProp("values") << "'"
				<< " for sensor " << itList.getProp("name")
				<< " in section <clickhouse> for " << it.getProp("name");
			throw uniset::SystemError(err.str());
		}

		auto ret = tags.emplace(myId,TagList());
		auto& lst = ret.first->second;
		auto lit = addTagsInfo( lst, sid );
		lit->vit = vit;
	}
}
//--------------------------------------------------------------------------------------------
ClickHouseTagsConfig::StateMap::iterator ClickHouseTagsConfig::addState( uniset::ObjectId sid, long defval )
{
	auto ret = smap.emplace(sid, defval);
	return ret.first;
}
//--------------------------------------------------------------------------------------------
ClickHouseTagsConfig::TagsMap ClickHouseTagsConfig::initTagsMap( UniXML::iterator it )
{
	if( !it.goChildren() )
	{
		ostringstream err;
		err << "(ClickHouseTagsConfig): not found items for initTagsMap " << endl;
		throw uniset::SystemError(err.str());
	}

	ClickHouseTagsConfig::TagsMap tmap;

	for( ; it.getCurrent(); ++ it )
	{
		std::string tname = it.getProp("name");
		if( tname.empty() )
		{
			ostringstream err;
			err << "(ClickHouseTagsConfig): not found 'name' for tag <" << it.getName() << ">" << endl;
			throw uniset::SystemError(err.str());
		}

		UniXML::iterator vit = it;
		if( !vit.goChildren() )
		{
			ostringstream err;
			err << "(ClickHouseTagsConfig): not found items for initTagsMap " << endl;
			throw uniset::SystemError(err.str());
		}

		ClickHouseTagsConfig::RangeList rlist = initRangeList(vit);
		tmap.emplace(tname, std::move(rlist));
//		tmap[tname] = rlist;
	}

	return tmap;
}
//--------------------------------------------------------------------------------------------
std::vector<ClickHouseTagsConfig::Tag> ClickHouseTagsConfig::parseTags( const std::string& s_tags, const char sep)
{
	std::vector<Tag> tags;

	if( sep == 0 )
	{
		auto tval = uniset::explode_str(s_tags,'=');
		if( tval.size() < 2 )
		{
			ostringstream err;
			err << "(ClickHouseTagsConfig::parseTags): bad tag value'" << s_tags << "'."
				<< " Must be 'key=value'";
			throw uniset::SystemError(err.str());
		}

		tags.push_back(Tag(tval[0],tval[1],tval[0],tval[1]));
	}
	else
	{
		auto tlist = uniset::explode_str(s_tags,sep);
		for( const auto& t: tlist )
		{
			auto tval = uniset::explode_str(t,'=');
			if( tval.size() < 2 )
			{
				ostringstream err;
				err << "(ClickHouseTagsConfig::parseTags): bad tag value'" << t << "'."
					<< " Must be 'key=value'";
				throw uniset::SystemError(err.str());
			}

			tags.push_back(Tag(tval[0],tval[1],tval[0],tval[1]));
		}
	}

	return tags;
}
//--------------------------------------------------------------------------------------------
ClickHouseTagsConfig::RangeList ClickHouseTagsConfig::initRangeList( UniXML::iterator it )
{
	ClickHouseTagsConfig::RangeList ranges;

	for( ; it.getCurrent(); ++it )
	{
		// example: range="[1,10]" or value="1"

		// init from 'value'
		string v = it.getProp("value");
		if( !v.empty() )
		{
			Range r;
			r.vmin = it.getIntProp("value");
			r.vmax = r.vmin;
			r.tags = parseTags(it.getProp("tags"),',');
			ranges.push_back(r);
			continue;
		}

		string s = it.getProp("range");
		if(s.empty())
		{
			ostringstream err;
			err << "(ClickHouseTagsConfig::initRangeList): Unknown range='' or value='' for tags='" << it.getProp("tags") << "'";
			throw uniset::SystemError(err.str());
		}

		// init from 'range'
		if( s[0] == '[' )
			s = s.substr(1,s.size());

		if( s[s.size() - 1] == ']' )
			s = s.substr(0,s.size()-1);

		auto l = uniset::explode_str(s,',');
		if( l.size() < 2 )
		{
			ostringstream err;
			err << "ClickHouseTagsConfig::initRangeList): Bad range='" << it.getProp("range") << "'."
				<< " Must be range='[min,max]'";

			throw uniset::SystemError(err.str());
		}

		Range r;

		if( l[0] == "min" )
			r.vmin = std::numeric_limits<long>::min();
		else
			r.vmin = uniset::uni_atoi(l[0]);

		if( l[1] == "max" )
			r.vmax = std::numeric_limits<long>::max();
		else
			r.vmax = uniset::uni_atoi(l[1]);

		if( r.vmin > r.vmax )
			std::swap(r.vmin,r.vmax);

//		r.tags = it.getProp("tags");
//		r.orig_tags = r.tags;
		r.tags = parseTags(it.getProp("tags"),' ');
		ranges.push_back(r);
	}

	return ranges;
}
//--------------------------------------------------------------------------------------------
ClickHouseTagsConfig::TagList::iterator ClickHouseTagsConfig::addTagsInfo( TagList& lst, uniset::ObjectId sid )
{
	for( TagList::iterator it = lst.begin(); it!=lst.end(); ++it )
	{
		if( it->sid == sid )
			return it;
	}

	TagsInfo inf;
	inf.sid = sid;
	inf.it = addState(sid,0);
	lst.push_back(inf);

	TagList::iterator it = lst.end();
	it--;
	return it;
}
//--------------------------------------------------------------------------------------------
bool ClickHouseTagsConfig::updateTags( uniset::ObjectId id, long value )
{
	auto it = smap.find(id);
	if( it != smap.end() )
	{
		it->second = value;
		return true;
	}

	return false;
}
//--------------------------------------------------------------------------------------------
size_t ClickHouseTagsConfig::getTagsCount() const
{
	return tagsmap.size();
}
//--------------------------------------------------------------------------------------------
size_t ClickHouseTagsConfig::getSensorsCount() const
{
	return smap.size();
}
//--------------------------------------------------------------------------------------------
std::vector<uniset::ObjectId> ClickHouseTagsConfig::getAskedSensors() const
{
	std::vector<uniset::ObjectId> lst;
	lst.reserve(smap.size()+tags.size());
	for( const auto& s: smap )
		lst.push_back(s.first);

	for( const auto& s: tags )
		lst.push_back(s.first);

	return lst;
}
//--------------------------------------------------------------------------------------------
std::vector<ClickHouseTagsConfig::Tag> ClickHouseTagsConfig::getTags( uniset::ObjectId id )
{
	auto it = tags.find(id);
	if( it == tags.end() )
		return std::vector<Tag>();

	return makeTags( it->second );
}
//--------------------------------------------------------------------------------------------
std::vector<ClickHouseTagsConfig::Tag> ClickHouseTagsConfig::makeTags( ClickHouseTagsConfig::TagList& lst )
{
	std::vector<Tag> tags;
	tags.reserve(lst.size()/2);

	for( auto&& t: lst )
	{
		if( !t.any_tags.empty() )
		{
			if( t.it->second != 0 )
			{
				for( const auto& a: t.any_tags )
					tags.emplace_back(a);
			}
		}
		else
		{
			// check ranges
			for( auto&& r: t.vit->second )
			{
				if( t.it->second != t.prev_value || !t.initOk )
					r.updateText(t.it->second);

				if( r.checkInRange(t.it->second) )
				{
					for( const auto& a: r.tags )
						tags.emplace_back(a);
				}
			}

			if( t.it->second != t.prev_value || !t.initOk )
			{
				t.initOk = true;
				t.prev_value = t.it->second;
			}
		}
	}

	tags.shrink_to_fit();
	return tags;
}
//--------------------------------------------------------------------------------------------
bool ClickHouseTagsConfig::Range::checkInRange( long value ) const
{
	return (value >= vmin && value <= vmax);
}
//--------------------------------------------------------------------------------------------
void ClickHouseTagsConfig::Range::updateText( long value )
{
	for( auto&& t: tags )
	{
		// optimization
		if( needReplaceKey )
		{
			t.key = uniset::replace_all(t.orig_key, "%v", std::to_string(value));
			t.key = uniset::replace_all(t.key, "%min", std::to_string(vmin));
			t.key = uniset::replace_all(t.key, "%max", std::to_string(vmax));

			if( t.key == t.orig_key )
				needReplaceKey = false;
		}

		if( needReplaceVal )
		{
			t.value = uniset::replace_all(t.orig_value, "%v", std::to_string(value));
			t.value = uniset::replace_all(t.value, "%min", std::to_string(vmin));
			t.value = uniset::replace_all(t.value, "%max", std::to_string(vmax));

			if( t.value == t.orig_value )
				needReplaceVal = false;
		}
	}
}
//--------------------------------------------------------------------------------------------
