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

	ins  []*uniset.Int64Value // список входов
	outs []*uniset.Int64Value // список выходов

	myname string
	id uniset.ObjectID
}

// ----------------------------------------------------------------------------------
func Init_<xsl:value-of select="$CLASSNAME"/>( sk* <xsl:value-of select="$CLASSNAME"/>, name string, section string ) {

	sk.myname = name

	jstr := uniset_internal_api.GetConfigParamsByName(name, section)
	cfg := uniset.UConfig{}
	bytes := []byte(jstr)

	err := json.Unmarshal(bytes, &amp;cfg)
	if err != nil {
		panic(fmt.Sprintf("(Init_<xsl:value-of select="$CLASSNAME"/>): error: %s", err))
	}

	sk.id = InitObjectID(&amp;cfg, name,name)

	<xsl:call-template name="VAR-INIT"/>
	<xsl:call-template name="ID-INIT"/>

	<xsl:call-template name="INS-INIT"/>
	<xsl:call-template name="OUTS-INIT"/>
}
// ----------------------------------------------------------------------------------
func PropValueByName( cfg *uniset.UConfig, propname string, defval string ) string {

	for _, v := range cfg.Config {

		if v.Prop == propname {
			return v.Value
		}
	}

	return defval
}
// ----------------------------------------------------------------------------------
func InitInt32( cfg *uniset.UConfig, propname string, defval string ) int32 {

	sval := PropValueByName(cfg,propname,defval)
	if len(sval) == 0 {
		return 0
	}

	i, err := strconv.ParseInt(sval, 10, 32)
	if err != nil{
		panic(fmt.Sprintf("(Init_Imitator_SK): convert type '%s' error: %s", propname, err))
	}

	return int32(i)
}
// ----------------------------------------------------------------------------------
func InitInt64( cfg *uniset.UConfig, propname string, defval string ) int64 {

	sval := PropValueByName(cfg,propname,defval)
	if len(sval) == 0 {
		return 0
	}

	i, err := strconv.ParseInt(sval, 10, 64)
	if err != nil{
		panic(fmt.Sprintf("(Init_Imitator_SK): convert type '%s' error: %s", propname, err))
	}

	return i
}
// ----------------------------------------------------------------------------------
func InitFloat32( cfg *uniset.UConfig, propname string, defval string ) float32 {

	sval := PropValueByName(cfg,propname,defval)
	if len(sval) == 0 {
		return 0.0
	}
	i, err := strconv.ParseFloat(sval, 32)
	if err != nil{
		panic(fmt.Sprintf("(Init_Imitator_SK): convert type '%s' error: %s", propname, err))
	}

	return float32(i)
}
// ----------------------------------------------------------------------------------
func InitFloat64( cfg *uniset.UConfig, propname string, defval string ) float64 {

	sval := PropValueByName(cfg,propname,defval)
	if len(sval) == 0 {
		return 0.0
	}

	i, err := strconv.ParseFloat(sval, 64)
	if err != nil{
		panic(fmt.Sprintf("(Init_Imitator_SK): convert type '%s' error: %s", propname, err))
	}

	return i
}
// ----------------------------------------------------------------------------------
func InitBool( cfg *uniset.UConfig, propname string, defval string ) bool {

	sval := PropValueByName(cfg,propname,defval)
	if len(sval) == 0 {
		return false
	}

	i, err := strconv.ParseBool(sval)
	if err != nil{
		panic(fmt.Sprintf("(Init_Imitator_SK): convert type '%s' error: %s", propname, err))
	}

	return i
}
// ----------------------------------------------------------------------------------
func InitString( cfg *uniset.UConfig, propname string, defval string ) string {

	return PropValueByName(cfg,propname,defval)
}
// ----------------------------------------------------------------------------------
func InitSensorID( cfg *uniset.UConfig, propname string, defval string ) uniset.SensorID {

	//fmt.Printf("init sensorID %s ret=%d\n",propname,uniset_internal_api.GetSensorID(PropValueByName(cfg,propname)))
	return uniset.SensorID(uniset_internal_api.GetSensorID(PropValueByName(cfg,propname,defval)))
}
// ----------------------------------------------------------------------------------
func InitObjectID( cfg *uniset.UConfig, propname string, defval string ) uniset.ObjectID {

	return uniset.ObjectID(uniset_internal_api.GetObjectID(PropValueByName(cfg,propname,defval)))
}
// ----------------------------------------------------------------------------------

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
<!--	<xsl:if test="normalize-space(@private)=''">
	<xsl:if test="normalize-space(@public)=''">
-->
	<xsl:if test="normalize-space(@type)!=''"></xsl:if>
	<xsl:if test="normalize-space(@type)='int'"><xsl:value-of select="@name"/> int32 /*!&lt; <xsl:value-of select="@comment"/> */
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
<!--
	</xsl:if>
	</xsl:if>
-->
	</xsl:for-each>
</xsl:template>

<xsl:template name="VAR-INIT">
	<xsl:for-each select="//variables/item">
<!--
	<xsl:if test="normalize-space(@private)=''">
	<xsl:if test="normalize-space(@public)=''">
-->
	<xsl:if test="normalize-space(@type)!=''">
	<xsl:if test="normalize-space(@type)='int'">sk.<xsl:value-of select="@name"/> = InitInt32(&amp;cfg,"<xsl:value-of select="@name"/>","<xsl:value-of select="@default"/>")
	</xsl:if>
	<xsl:if test="normalize-space(@type)='long'">sk.<xsl:value-of select="@name"/> =  InitInt64(&amp;cfg,"<xsl:value-of select="@name"/>","<xsl:value-of select="@default"/>")
	</xsl:if>
	<xsl:if test="normalize-space(@type)='float'">sk.<xsl:value-of select="@name"/> =  InitFloat32(&amp;cfg,"<xsl:value-of select="@name"/>","<xsl:value-of select="@default"/>")
	</xsl:if>
	<xsl:if test="normalize-space(@type)='double'">sk.<xsl:value-of select="@name"/> =  InitFloat64(&amp;cfg,"<xsl:value-of select="@name"/>","<xsl:value-of select="@default"/>")
	</xsl:if>
	<xsl:if test="normalize-space(@type)='bool'">sk.<xsl:value-of select="@name"/> = InitBool(&amp;cfg,"<xsl:value-of select="@name"/>","<xsl:value-of select="@default"/>")
	</xsl:if>
	<xsl:if test="normalize-space(@type)='str'">sk.<xsl:value-of select="@name"/> =  InitString(&amp;cfg,"<xsl:value-of select="@name"/>","<xsl:value-of select="@default"/>")
	</xsl:if>
	<xsl:if test="normalize-space(@type)='sensor'">sk.<xsl:value-of select="@name"/> = InitSensorID(&amp;cfg,"<xsl:value-of select="@name"/>","<xsl:value-of select="@default"/>")
	</xsl:if>
	<xsl:if test="normalize-space(@type)='object'">sk.<xsl:value-of select="@name"/> =  InitObjectID(&amp;cfg,"<xsl:value-of select="@name"/>","<xsl:value-of select="@default"/>")
	</xsl:if>
	</xsl:if>
<!--
	</xsl:if>
	</xsl:if>
-->
	</xsl:for-each>

</xsl:template>

<xsl:template name="IO-LIST">
	<xsl:for-each select="//smap/item"><xsl:if test="normalize-space(@vartype)='in'"><xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> int64
	</xsl:if>
	<xsl:if test="normalize-space(@vartype)='out'"><xsl:call-template name="setprefix"/><xsl:value-of select="@name"/> int64
	</xsl:if>
	</xsl:for-each>
</xsl:template>

<xsl:template name="ID-LIST">
	<xsl:for-each select="//smap/item"><xsl:value-of select="@name"/> uniset.SensorID
	</xsl:for-each>
</xsl:template>

<xsl:template name="ID-INIT">
	<xsl:for-each select="//smap/item">sk.<xsl:value-of select="@name"/> = InitSensorID(&amp;cfg,"<xsl:value-of select="@name"/>","<xsl:value-of select="@default"/>")
	</xsl:for-each>
</xsl:template>

<xsl:template name="INS-INIT">
	sk.ins = []*uniset.Int64Value {
	<xsl:for-each select="//smap/item">
	<xsl:if test="normalize-space(@vartype)='in'">uniset.NewInt64Value(&amp;sk.<xsl:value-of select="@name"/>,&amp;sk.<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>),
	</xsl:if>
	</xsl:for-each>}
</xsl:template>
<xsl:template name="OUTS-INIT">
	sk.outs = []*uniset.Int64Value {
	<xsl:for-each select="//smap/item">
	<xsl:if test="normalize-space(@vartype)='out'">uniset.NewInt64Value(&amp;sk.<xsl:value-of select="@name"/>,&amp;sk.<xsl:call-template name="setprefix"/><xsl:value-of select="@name"/>),
	</xsl:if>
	</xsl:for-each>}
</xsl:template>

</xsl:stylesheet>
