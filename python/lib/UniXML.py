#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import libxml2
import xml.dom.minidom as md
import re
import os
# ----------------------------- 
class EmptyNode():
     def __init__(self):
         self.proplist = dict()

     def prop(self,name):
         if name in self.proplist:
            return self.proplist[name]

         return ""

     def setProp(self,name,val):
         self.proplist[name] = val

     def reset(self):
         self.proplist = dict()

# -----------------------------
class UniXMLException(Exception):
    
    def __init__(self,e=""):
        self.err = e
    
    def getError(self):
        return self.err
# -----------------------------
class UniXML():
    
    def __init__(self, xfile, isDoc=False):
        try:
            self.doc = None
            if isDoc:
               self.fname = ''
               self.doc = libxml2.parseDoc(xfile)
            else:
               self.fname = xfile
               self.doc = libxml2.parseFile(xfile)
        except libxml2.parserError:
            raise UniXMLException("(UniXML): Can`t open file " + xfile)
        
        libxml2.registerErrorHandler(self.callback, "-->")

    def __del__(self):
        if self.doc != None:
            self.doc.freeDoc()
        libxml2.cleanupParser()
        if libxml2.debugMemory(1) != 0:
            print "Memory leak %d bytes" % (libxml2.debugMemory(1))
            libxml2.dumpMemory()
        
    def callback(ctx, str):
        print "%s %s" % (ctx, str)

    def getDoc(self):
        return self.doc

    def getFileName(self):
        return self.fname

    def findNode(self, node, nodestr="", propstr=""):
        while node != None:
            if node.name == nodestr:
                return [node, node.name, node.prop(propstr)]
            ret = self.findNode(node.children, nodestr, propstr)
            if ret[0] != None:
                return ret
            node = node.next
        return [None, None, None]

    def findMyLevel(self, node, nodestr="", propstr=""):
        while node != None:
            if node.name == nodestr:
                return [node, node.name, node.prop(propstr)]
            node = node.next
        return [None, None, None]
    
    def findNode_byProp(self, node, propstr, valuestr):
        while node != None:
            if node.prop(propstr) == valuestr:
                return [node, node.name, node.prop(propstr)]
            ret = self.findNode_byProp (node.children, propstr, valuestr)
            if ret[0] != None:
                return ret
            node = node.next
        return [None, None, None]

    def nextNode(self, node):
        while node != None:
            node = node.next
            if node == None:
                return node
            if node.name != "text" and node.name != "comment":
                  return node
        return None

    def getNode(self, node):
        if node == None:
           return None
        if node.name != "text" and node.name != "comment":
           return node
        return self.nextNode(node)

    def firstNode(self, node):
        while node != None:
            prev = node
            node = node.prev
            if node == None:
                node = prev
                break
        return self.getNode(node)

    def lastNode(self, node):
        prev = node
        while node != None:
           prev = node
           node = self.nextNode(node)

        return prev

    def getProp(self, node, str):
        prop = node.prop(str)
        if prop != None:
            return prop
        else:
            return ""

    def save(self, filename=None, pretty_format=True, backup=False):
        if filename == None:
           filename = self.fname

        if backup:
           # чтобы "ссылки" не бились, делаем копирование не через rename
           #os.rename(self.fname,str(self.fname+".bak"))
           f_in = file(self.fname,'rb')
           f_out = file(str(self.fname+".bak"),'wb')
           f_in.seek(0)
           f_out.write(f_in.read())
           f_in.close()
           f_out.close()

        if pretty_format == True:
           return self.pretty_save(filename)

        return self.doc.saveFile(filename)

    def reopen(self, filename, isDoc=False):
        try:
            self.doc.freeDoc()
            if isDoc:
                self.fname = ''
                self.doc = libxml2.parseDoc(filename)
            else:
                self.fname = filename
                self.doc = libxml2.parseFile(filename)
        except libxml2.parserError:
            sys.exit(-1)

    def pretty_save(self, filename):
        context = self.doc.serialize(encoding="utf-8")
        mdoc = md.parseString(context) 
        s = mdoc.toprettyxml(encoding="utf-8").split("\n")
        out = open(filename,"w")
        p = re.compile(r'\ [^\s]{1,}=""')
        for l in s:
            if l.strip():
               # удаяем пустые свойства prop=""
               l = p.sub('', l)
               out.write(l+"\n")
        out.close()
