// --------------------------------------------------------------------------
#ifndef ClickHouseTagsConfig_H_
#define ClickHouseTagsConfig_H_
// --------------------------------------------------------------------------
#include <memory>
#include <unordered_map>
#include <string>
#include "Configuration.h"
//------------------------------------------------------------------------------------------
namespace uniset
{
    //------------------------------------------------------------------------------------------
    /*! \page page_ClickHouseTags BackendClickHouse: Поддержка динамических тегов

      "Динамические теги" это добавление тегов в зависимости от состояния указанного датчика.
      При этом можно указывать любые датчики. Значения тегов могут быть статическими и динамическими
      (т.е. формироваться на основе значения).

      Доступно три способа настройки тегов

      \section pgClickHouseTagsConfig_dyn_tags Прямое указание статических значений тегов
      \code
      <sensor name="Sensor1" iotype="AI" ... clickhouse_dyn_tags="key1=tag1 key2=tag2" clickhouse_dyn_sid="Sensor3" />
      \endcode
      Где
      - clickhouse_dyn_sid - задаёт датчик на который реагировать (!=0)
      - clickhouse_dyn_tags - задаёт список тегов, которые будут добавлены если датчик != 0

      \section pgClickHouseTagsConfig_dyn_values Указание динамических тегов при использовании только одного датчика
      \code
      <sensor name="Sensor1" iotype="AI" ... clickhouse_dyn_values="modes" clickhouse_dyn_sid="Sensor3" />
      \endcode
      Где
      - clickhouse_dyn_sid - задаёт датчик на который реагировать (!=0)
      - clickhouse_dyn_values - задаёт название секции привязок в секции '<clickhouse_tags>' (см. \ref pgClickHouseTagsConfig_clickhouse )

      \section pgClickHouseTagsConfig_dyn_section Указание динамических тегов формирующихся от нескольких датчиков
      Для более тонкой настройки задаётся имя специальной секции, в которой теги можно сопоставить конкретному значению датчика
      \code
      <sensor name="Sensor1" iotype="AI"..>
      <clickhouse_tags>
          <sensor name="S1" values="modes"/>
          <sensor name="S2" values="myTag2"/>
      </clickhouse_tags>
      </sensor>
      \endcode
      Где
      - name - задаёт название датчиков
      - values - задаёт название секции привязок в секции '<clickhouse_tags>' (см. \ref pgClickHouseTagsConfig_clickhouse )

      \section pgClickHouseTagsConfig_clickhouse Секция настройки сопоставления значений и тегов
      Для сопоставления значений и тегов в configure.xml должна быть специальная секция
      \code
      <clickhouse_tags>
         <tag name="modes">
            <item value="1" tags="key1=tag1 key2=tag2"/>
            <item value="2" tags="key3=tag3 key4=tag4"/>
            <item range="[3,100]" tags="key5=tag5 key6=tag6"/>
         </tag>
         <tag name="myTag2">
            <item value="1" tags="mykey1=mytag1 mykey2=mytag2"/>
            <item value="2" tags="mykey3=mytag3 mykey4=mytag4"/>
            <item value="4" tags="mykey5=mytag5 mykey6=mytag6"/>
            <item range="[min,10]" tags="mykey7=mytag8 mykey8=mytag8"/>
         </tag>
         <tag name="myDynValue">
            <item range="[min,max]" tags="mykey=%v key2=%min key3=text"/>
            <item range="[10,15]" tags="mykey=%min_%v_%max"/>
         </tag>
      </clickhouse_tags>
      \endcode
      Поддерживается два способа задания диапазона.
      - <b>"value"</b> - прямое указание значения
      - <b>"range"</b> - указание диапазона. При этом разрешено указывать в качестве значений "min" и "max".
      При этом будут использованы минимально и максимально возможные значения для типа long.

      Помимо этого поддерживается возможность динамически формировать тег.
      Возможные следующие замены:
      - <b>\%v</b> - текущее значение
      - <b>\%min</b> - минимум указанный в "range"
      - <b>\%max</b> - максимум указанный в "range"

      min, min - действуют только для "range".

     */
    class ClickHouseTagsConfig
    {

        public:
            ClickHouseTagsConfig();
            ~ClickHouseTagsConfig();

            // init all tags from configure
            void load( const std::shared_ptr<uniset::Configuration> conf
                       , const std::string filter_field = ""
                       , const std::string filter_value = "" );


            // init tags map from section <cnodename>
            void loadTagsMap( uniset::UniXML::iterator it );

            // init tags for sensor
            void initFromItem( const std::shared_ptr<uniset::Configuration> conf
                               , const uniset::UniXML::iterator& it );

            struct Tag
            {
                std::string key;
                std::string value;
                const std::string orig_key;
                const std::string orig_value;

                Tag( const std::string& _key
                     , const std::string& _val
                     , const std::string& _orig_key
                     , const std::string& _orig_val):
                    key(_key),
                    value(_val),
                    orig_key(_orig_key),
                    orig_value(_orig_val)
                {}
            };

            // parse string "key1=val1 key2=val2 key3=val3"
            static std::vector<Tag> parseTags( const std::string& s_tags, const char sep = ' ');

            std::vector<Tag> getTags( uniset::ObjectId id );

            bool updateTags( uniset::ObjectId id, long value );

            size_t getTagsCount() const;
            size_t getSensorsCount() const;

            using StateMap = std::unordered_map<uniset::ObjectId, long>;

            std::vector<uniset::ObjectId> getAskedSensors() const;

        protected:

            // текущее значение датчиков по которым формируются теги (map<id,value>)
            StateMap::iterator addState( uniset::ObjectId sid, long defval );

            StateMap smap;

            struct Range
            {
                long vmin;
                long vmax;
                bool checkInRange( long value ) const;
                void updateText( long value );
                std::vector<Tag> tags;

                bool needReplaceKey = {true};
                bool needReplaceVal = {true};
            };

            // value --> tags
            using RangeList = std::list<Range>;

            RangeList initRangeList( uniset::UniXML::iterator it );

            // name --> range list
            using TagsMap  = std::unordered_map<std::string, RangeList>;
            TagsMap tagsmap; // список тегов
            TagsMap initTagsMap( uniset::UniXML::iterator it );

            struct TagsInfo
            {
                std::vector<Tag> any_tags; // if not empty, work for any value
                TagsMap::iterator vit;
                StateMap::iterator it;
                long prev_value = { 0 };
                bool initOk = { false };
                uniset::ObjectId sid = { uniset::DefaultObjectId };
            };

            using TagList = std::list<TagsInfo>;
            TagList::iterator addTagsInfo( TagList& lst, uniset::ObjectId sid );
            std::vector<Tag> makeTags( TagList& lst );

            // map<sensorID,TagList>
            std::unordered_map<uniset::ObjectId, TagList> tags;

            void initDynamicTags();
            void initFromTags( const std::shared_ptr<uniset::Configuration>& conf, uniset::UniXML::iterator it, const std::string s_tags );
            void initFromValues( const std::shared_ptr<uniset::Configuration>& conf, uniset::UniXML::iterator it, const std::string s_values );
            void initFromTagsList( const std::shared_ptr<uniset::Configuration>& conf, uniset::UniXML::iterator it, uniset::UniXML::iterator itList );
    };
    //------------------------------------------------------------------------------------------
} // end of namespace Theatre
//------------------------------------------------------------------------------------------
#endif
