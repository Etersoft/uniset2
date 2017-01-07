#!/usr/bin/env python
# -*- coding: utf-8 -*-

from pyUConnector import *
import time
import sys

def is_id( str_id ):
    if isinstance(str_id,int) or isinstance(str_id,long):
       return True

    if str_id.strip().isdigit():
       return True

    return False

def to_int(s):

    if s == None or s == "":
       return 0

    if isinstance(s,int) or isinstance(s,long):
       return s

    if isinstance(s,float):
       return int(s)

    if len(s)>2 and s[0] == '0' and s[1] == 'x':
       return int(s,16)

    if s.upper() == "TRUE":
       return 1

    if s.upper() == "FALSE":
       return 0

    return int(s.strip())
    
def to_str(s_val):

    if s_val == None:
       return ""

    return str(s_val)

def bool2str( state ):
    if state:
       return "1"
    return "0"

def get_sinfo(raw, sep='@'):
    raw = to_str(raw)
    v = raw.strip().split(sep)
    if len(v) > 1:
       return [v[0],v[1]]
    return [v[0],""]

# Функция требует инициализированного UConnector
# (т.е. загруженного configure.xml)
def to_sid(str_id, ui):
    if str_id == None or str_id == "":
       return [DefaultID,DefaultID,""]
    
    if is_id(str_id):
       return [int(str_id),DefaultID,ui.getShortName(int(str_id))]
    
    s = get_sinfo(str_id)
    s_id = s[0]
    s_node = s[1]
   
    if s_id == "":
       return [DefaultID,DefaultID,"?@?"]
    
    s_name = ""
    if is_id(s_id):
       s_id = int(s_id)
       s_name = ui.getShortName(s_id)
    else:
       s_name = s_id
       s_id = ui.getSensorID(s_id)
    
    if s_node == "":
       return [s_id,DefaultID,s_name]
    
    n_name = ""
    if is_id(s_node):
       s_node = int(s_node)
       n_name = ui.getShortName(s_node)
    else:
       n_name = s_node
       s_node = ui.getNodeID(s_node)
    
    return [s_id,s_node,str(s_name+"@"+n_name)]

# Получение списка пар [id@node,int(val)] из строки "id1@node1=val1,id2=val2,.."
def get_int_list(raw_str,sep='='):
    
    if raw_str == None or raw_str == "":
       return []
    
    slist = []
    l = raw_str.split(",")
    for s in l:
        v = s.split(sep)
        if len(v) > 1:
           slist.append([v[0],to_int(v[1])])
        else:
           print "(get_list:WARNING): (v=x) undefined value for " + str(s)
           slist.append([v[0],0])
    return slist

def list_to_str(lst,sep='='):
    res = ""
    for v in lst:
        if res != "":
           res += ",%s=%s"%(v[0],v[1])
        else:
           res += "%s=%s"%(v[0],v[1])
    
    return res

# Получение списка пар [sX,kX] из строки "s1=k1,s2=k2,.."
def get_str_list(raw_str,sep='='):
    
    if raw_str == None or raw_str == "":
       return []
    
    slist = []
    l = raw_str.split(",")
    for s in l:
        v = s.split("=")
        if len(v) > 1:
           slist.append([v[0],v[1]])
        else:
           print "(get_str_list:WARNING): (v=x) undefined value for " + str(s)
           slist.append([v[0],""])
    return slist

# Получение списка пар [key,val] из строки "key1:val1,key2:val2,.."
def get_replace_list(raw_str):
    
    if raw_str == None or raw_str == "":
       return []
    slist = []
    l = raw_str.split(",")
    for s in l:
        v = s.split(":")
        if len(v) > 1:
           key = to_str(v[0]).strip().strip("\n")
           val = to_str(v[1]).strip().strip("\n")
           slist.append([key,val])
        else:
           print "(get_replace_list:WARNING): (v:x) undefined value for " + str(s)
           key = to_str(v[0]).strip().strip("\n")
           slist.append([key,0])
    
    return slist

# Парсинг строки вида hostname:port
def get_mbslave_param( raw_str, sep=':' ):

    if to_str(raw_str) == "":
       return [None,None]

    l = raw_str.split(sep)
    if len(l) > 2:
       print "(get_mbslave_param:WARNING): BAD FORMAT! string='%s'. Must be 'hostname:port'"%(raw_str)
       return [None,None]

    if len(l) == 2:
       return [ l[0], to_int(l[1]) ]

    #default modbus port = 502
    return [ l[1], 502 ]

# Парсинг строки вида "mbreg@mbaddr:mbfunc:nbit:vtype"
def get_mbquery_param( raw_str, def_mbfunc="0x04", ret_str=False ):

    raw_str = to_str(raw_str)

    l = raw_str.split(':')

    vtype = "signed"
    if len(l) > 3:
       vtype = l[3]

    nbit = -1
    if len(l) > 2:
       nbit = l[2]

    mbfunc = def_mbfunc
    if len(l) > 1:
       mbfunc = l[1]

    if mbfunc.startswith("0x"):
       mbfunc = str(int(mbfunc,16))
    else:
       mbfunc = str(int(mbfunc,10))

    mbreg = None
    mbaddr = "0x01"
    if len(l) > 0:
       p = l[0].split('@')
       mbreg = p[0]
       if len(p) > 1:
          mbaddr = p[1]

    if len(mbaddr) == 0 or len(mbreg) == 0 or len(mbfunc) == 0:
       if ret_str == True:
          return ["","","","",""]
          
       #print "(get_mbquery_param:WARNING): BAD STRING FORMAT! strig='%s'. Must be 'mbreg@mbaddr:mbfunc:nbit:vtype'"%(raw_str)
       return [None,None,None,None,None]

    if ret_str == False:
       return [ to_int(mbaddr), to_int(mbreg), to_int(mbfunc), to_int(nbit), vtype ]

    return [ mbaddr, mbreg, mbfunc, nbit, vtype ]

def fcalibrate( raw, rawMin, rawMax, calMin, calMax ):

    if rawMax == rawMin:
       return 0; # деление на 0!!!

    return 1.0 * (raw - rawMin) * (calMax - calMin) / ( rawMax - rawMin ) + calMin;

def getArgParam(param,defval=""):
    for i in range(0, len(sys.argv)):
        if sys.argv[i] == param:
            if i+1 < len(sys.argv):
                return sys.argv[i+1]
            else:
                break;
            
    return defval
    
def getArgInt(param,defval=0):
    for i in range(0, len(sys.argv)):
        if sys.argv[i] == param:
            if i+1 < len(sys.argv):
                return to_int(sys.argv[i+1])
            else:
                break;
        
    return defval
    
def checkArgParam(param,defval=""):
    for i in range(0, len(sys.argv)):
        if sys.argv[i] == param:
            return True
        
    return defval
