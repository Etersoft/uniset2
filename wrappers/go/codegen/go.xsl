<?xml version='1.0' encoding="utf-8" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version='1.0'
		             xmlns:date="http://exslt.org/dates-and-times">

<xsl:output method="text" indent="yes" encoding="utf-8"/>

<xsl:variable name="CLASSNAME">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'class-name'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="BASECLASS">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'base-class'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="OID">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'ID'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="TESTMODE">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'testmode'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="ARGPREFIX">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'arg-prefix'"/></xsl:call-template>
</xsl:variable>
<xsl:variable name="PACKAGE">
	<xsl:call-template name="settings"><xsl:with-param name="varname" select="'package'"/></xsl:call-template>
</xsl:variable>

<xsl:template match="/">

<xsl:if test="normalize-space($PACKAGE)=''">
package main
</xsl:if>
<xsl:if test="normalize-space($PACKAGE)!=''">
package <xsl:value-of select="$PACKAGE"/>
</xsl:if>

import (
	"fmt"
	"uniset"
	"reflect"
	"uniset_internal_api"
	"encoding/json"
	"strconv"
)

// ----------------------------------------------------------------------------------
type  <xsl:value-of select="$CLASSNAME"/>_SK struct {

	// ID
	<xsl:call-template name="ID-LIST"/>

	// i/o
	<xsl:call-template name="IO-LIST"/>

	// variables
	<xsl:call-template name="VAR-LIST"/>

	din  []*uniset.BoolValue  // список bool-вых входов
	ain  []*uniset.Int64Value // список аналоговых входов
	dout []*uniset.BoolValue  // список bool-вых выходов
	aout []*uniset.Int64Value // список аналоговых выходов

}

// ----------------------------------------------------------------------------------
func Init_<xsl:value-of select="$CLASSNAME"/>( sk* <xsl:value-of select="$CLASSNAME"/>, name string, section string ) {

	jstr := uniset_internal_api.GetConfigParamsByName(name, section)
	cfg := uniset.UConfig{}
	bytes := []byte(jstr)

	err := json.Unmarshal(bytes, &amp;cfg)
	if err != nil {
		panic(fmt.Sprintf("(Init_<xsl:value-of select="$CLASSNAME"/>): error: %s", err))
	}

	for _, v := range cfg.Config {

		f := reflect.ValueOf(sk).Elem().FieldByName(v.Prop)
		if f.IsValid() {
			switch t := f.Interface().(type) {

			case int:
			case int64:
				i, err := strconv.ParseInt(v.Value, 10, 64)
				if err != nil{
					panic(fmt.Sprintf("(Init_<xsl:value-of select="$CLASSNAME"/>): convert type '%s' error: %s", v.Value, err))
				}
				f.SetInt(i)

			case float32:
				i, err := strconv.ParseFloat(v.Value, 32)
				if err != nil{
					panic(fmt.Sprintf("(Init_<xsl:value-of select="$CLASSNAME"/>): convert type '%s' error: %s", v.Value, err))
				}
				f.SetFloat(i)

			case float64:
				i, err := strconv.ParseFloat(v.Value, 64)
				if err != nil{
					panic(fmt.Sprintf("(Init_<xsl:value-of select="$CLASSNAME"/>): convert type '%s' error: %s", v.Value, err))
				}
				f.SetFloat(i)

			case string:
				f.SetString(v.Value)

			case bool:
				i, err := strconv.ParseBool(v.Value)
				if err != nil{
					panic(fmt.Sprintf("(Init_<xsl:value-of select="$CLASSNAME"/>): convert type '%s' error: %s", v.Value, err))
				}
				f.SetBool(i)

			case uniset.SensorID:
			case uniset.ObjectID:
				i, err := strconv.ParseInt(v.Value, 10, 64)
				if err != nil{
					panic(fmt.Sprintf("(Init_<xsl:value-of select="$CLASSNAME"/>): convert type '%s' error: %s", v.Value, err))
				}
				f.SetInt(i)

			default:
				panic(fmt.Sprintf("(Init_<xsl:value-of select="$CLASSNAME"/>): Unsupported type of value: %s", t))
			}
		}
	}

<!--	im.aout = []*uniset.Int64Value{uniset.NewInt64Value(&im.level_s, &im.out_level_s)}

	im.din = []*uniset.BoolValue{
		uniset.NewBoolValue(&im.switchOn_c, &im.in_switchOn_c),
		uniset.NewBoolValue(&im.switchOff_c, &im.in_switchOff_c)}

	return &im-->
}

</xsl:template>


<xsl:template name="setprefix">
<xsl:choose>
	<xsl:when test="normalize-space(@vartype)='in'">in_</xsl:when>
	<xsl:when test="normalize-space(@vartype)='out'">out_</xsl:when>
	<xsl:when test="normalize-space(@vartype)='none'">nn_</xsl:when>
	<xsl:when test="normalize-space(@vartype)='io'">NOTSUPPORTED_IO_VARTYPE_</xsl:when>
</xsl:choose>
</xsl:template>

<xsl:template name="settings">
	<xsl:param name="varname"/>
	<xsl:for-each select="//settings">
		<xsl:for-each select="./*">
			<xsl:if test="normalize-space(@name)=$varname"><xsl:value-of select="@val"/></xsl:if>
		</xsl:for-each>
	</xsl:for-each>
</xsl:template>

<xsl:template name="VAR-LIST">
	<xsl:for-each select="//variables/item">
	<xsl:if test="normalize-space(@private)=''">
	<xsl:if test="normalize-space(@public)=''">
	<xsl:if test="normalize-space(@type)!=''"><xsl:if test="normalize-space(@const)!=''">const </xsl:if></xsl:if>
	<xsl:if test="normalize-space(@type)='int'"><xsl:value-of select="@name"/> int /*!&lt; <xsl:value-of select="@comment"/> */
	</xsl:if>
	<xsl:if test="normalize-space(@type)='long'"><xsl:value-of select="@name"/> int64 /*!&lt; <xsl:value-of select="@comment"/> */
	</xsl:if>
	<xsl:if test="normalize-space(@type)='float'"><xsl:value-of select="@name"/> float32 /*!&lt; <xsl:value-of select="@comment"/> */
	</xsl:if>
	<xsl:if test="normalize-space(@type)='double'"><xsl:value-of select="@name"/> float64 /*!&lt; <xsl:value-of select="@comment"/> */
	</xsl:if>
	<xsl:if test="normalize-space(@type)='bool'"><xsl:value-of select="@name"/> bool /*!&lt; <xsl:value-of select="@comment"/> */
	</xsl:if>
	<xsl:if test="normalize-space(@type)='str'"><xsl:value-of select="@name"/> string /*!&lt; <xsl:value-of select="@comment"/> */
	</xsl:if>
	<xsl:if test="normalize-space(@type)='sensor'"><xsl:value-of select="@name"/> uniset.SensorID /*!&lt; <xsl:value-of select="@comment"/> */
	</xsl:if>
	<xsl:if test="normalize-space(@type)='object'"><xsl:value-of select="@name"/> uniset.ObjectID /*!&lt; <xsl:value-of select="@comment"/> */
	</xsl:if>
	</xsl:if>
	</xsl:if>
	</xsl:for-each>
</xsl:template>

<xsl:template name="IO-LIST">
	<xsl:for-each select="//smap/item"><xsl:if test="normalize-space(@vartype)='in'"><xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> uniset.SensorID
	</xsl:if>
	<xsl:if test="normalize-space(@vartype)='out'"><xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> uniset.SensorID
	</xsl:if>
	</xsl:for-each>
</xsl:template>

<xsl:template name="ID-LIST">
	<xsl:for-each select="//smap/item"><xsl:value-of select="@name"/> uniset.SensorID
	</xsl:for-each>
</xsl:template>



<xsl:template name="default-init-variables">
<xsl:if test="normalize-space(@const)!=''">
<xsl:if test="normalize-space(@type)='int'"><xsl:value-of select="normalize-space(@name)"/>(0),
</xsl:if>
<xsl:if test="normalize-space(@type)='long'"><xsl:value-of select="normalize-space(@name)"/>(0),
</xsl:if>
<xsl:if test="normalize-space(@type)='float'"><xsl:value-of select="normalize-space(@name)"/>(0),
</xsl:if>
<xsl:if test="normalize-space(@type)='double'"><xsl:value-of select="normalize-space(@name)"/>(0),
</xsl:if>
<xsl:if test="normalize-space(@type)='bool'"><xsl:value-of select="normalize-space(@name)"/>(false),
</xsl:if>
<xsl:if test="normalize-space(@type)='str'"><xsl:value-of select="normalize-space(@name)"/>(""),
</xsl:if>
<xsl:if test="normalize-space(@type)='sensor'"><xsl:value-of select="normalize-space(@name)"/>(DefaultObjectId),
</xsl:if>
<xsl:if test="normalize-space(@type)='object'"><xsl:value-of select="normalize-space(@name)"/>(DefaultObjectId),
</xsl:if>
</xsl:if>
</xsl:template>

<xsl:template name="init-variables">
<xsl:if test="normalize-space(@type)='int'">
<xsl:value-of select="normalize-space(@name)"/>(uni_atoi( init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>"))),
</xsl:if>
<xsl:if test="normalize-space(@type)='long'">
<xsl:value-of select="normalize-space(@name)"/>(uni_atoi( init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>"))),
</xsl:if>
<xsl:if test="normalize-space(@type)='float'">
<xsl:value-of select="normalize-space(@name)"/>(atof( init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>").c_str())),
</xsl:if>
<xsl:if test="normalize-space(@type)='double'">
<xsl:value-of select="normalize-space(@name)"/>(atof( init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>").c_str())),
</xsl:if>
<xsl:if test="normalize-space(@type)='bool'">
<xsl:value-of select="normalize-space(@name)"/>(uni_atoi( init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>"))),
</xsl:if>
<xsl:if test="normalize-space(@type)='str'">
<xsl:value-of select="normalize-space(@name)"/>(init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>")),
</xsl:if>
<xsl:if test="normalize-space(@type)='sensor'">
<xsl:value-of select="normalize-space(@name)"/>(uniset_conf()->getSensorID(init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>"))),
</xsl:if>
<xsl:if test="normalize-space(@type)='object'">
<xsl:value-of select="normalize-space(@name)"/>(uniset_conf()->getObjectID(init3_str(uniset_conf()->getArgParam("--" + (_argprefix.empty() ? myname+"-" : _argprefix) + "<xsl:value-of select="@name"/>"),uniset_conf()->getProp(cnode,"<xsl:value-of select="@name"/>"),"<xsl:value-of select="normalize-space(@default)"/>"))),
</xsl:if>
</xsl:template>

</xsl:stylesheet>
