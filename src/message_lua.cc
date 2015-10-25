/**
 * $FILENAME - $TITLE
 *
 * This file is part of lumail - http://lumail.org/
 *
 * Copyright (c) 2015 by Steve Kemp.  All rights reserved.
 *
 **
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 dated June, 1991, or (at your
 * option) any later version.
 *
 * On Debian GNU/Linux systems, the complete text of version 2 of the GNU
 * General Public License can be found in `/usr/share/common-licenses/GPL-2'
 */


extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>
#include <gmime/gmime.h>

#include "message.h"
#include "message_part.h"


/**
 * Binding for CMessage
 */
int l_CMessage_constructor(lua_State * l)
{
    const char *name = luaL_checkstring(l, 1);

    CMessage **udata = (CMessage **) lua_newuserdata(l, sizeof(CMessage *));
    *udata = new CMessage(name);

    luaL_getmetatable(l, "luaL_CMessage");

    lua_setmetatable(l, -2);

    return 1;
}

/**
 * Test that the object is a CMessage.
 */
CMessage * l_CheckCMessage(lua_State * l, int n)
{
    return *(CMessage **) luaL_checkudata(l, n, "luaL_CMessage");
}

/**
 * Get the path to the message, on-disk.
 */
int l_CMessage_path(lua_State * l)
{
    CMessage *foo = l_CheckCMessage(l, 1);

    lua_pushstring(l, foo->path().c_str());
    return 1;
}

/**
 * Get the value of a specific header.
 */
int l_CMessage_header(lua_State * l)
{
    CMessage *foo = l_CheckCMessage(l, 1);

    /* Get the header. */
    const char *str = luaL_checkstring(l, 2);
    std::string result = foo->header(str);

    /* set the retulr */
    lua_pushstring(l, result.c_str());
    return 1;

}

/**
 * Return all header names & values.
 */
int l_CMessage_headers(lua_State * l)
{
    /**
     * Get the headers.
     */
    CMessage *foo = l_CheckCMessage(l, 1);
    std::unordered_map < std::string, std::string > headers = foo->headers();


    /**
     * Create the table.
     */
    lua_newtable(l);

    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        std::string name = it->first;
        std::string value = it->second;

        lua_pushstring(l, name.c_str());

        if (!value.empty())
            lua_pushstring(l, value.c_str());
        else
            lua_pushstring(l, "[EMPTY]");

        lua_settable(l, -3);
    }

    return (1);
}

/**
 * Return an array of CMessagePart objects to Lua.  These can be inspected
 * as the user wishes.
 *
 * NOTE: CMessagePart is *NOT* creatable via Lua.  This is a good thing.
 *
 */
int l_CMessage_parts(lua_State * l)
{
    CMessage *foo = l_CheckCMessage(l, 1);

    /**
     * Get the parts, and count.
     */
    std::vector < CMessagePart * >parts = foo->get_parts();

    lua_createtable(l, parts.size(), 0);
    int i = 0;

    for (std::vector < CMessagePart * >::iterator it = parts.begin();
            it != parts.end(); ++it)
    {
        CMessagePart **udata =
            (CMessagePart **) lua_newuserdata(l, sizeof(CMessagePart *));
        *udata = (*it);
        luaL_getmetatable(l, "luaL_CMessagePart");
        lua_setmetatable(l, -2);
        lua_rawseti(l, -2, i + 1);

        i++;
    }

    return 1;
}

/**
 * Get/Set the flags.
 */
int l_CMessage_flags(lua_State * l)
{
    CMessage *foo = l_CheckCMessage(l, 1);

    /**
     * Are we setting the flags?
     */
    int n = lua_gettop(l);

    if (n > 1)
    {
        const char *update = luaL_checkstring(l, 1);
        foo->set_flags(update);
    }

    /**
     * Now get the flags
     */
    lua_pushstring(l, foo->get_flags().c_str());
    return 1;
}

/**
 * Destructor.
 */
int l_CMessage_destructor(lua_State * l)
{
    CMessage *foo = l_CheckCMessage(l, 1);
    delete foo;

    return 0;
}

void InitMessage(lua_State * l)
{
    luaL_Reg sFooRegs[] =
    {
        {"new", l_CMessage_constructor},
        {"path", l_CMessage_path},
        {"parts", l_CMessage_parts},
        {"header", l_CMessage_header},
        {"headers", l_CMessage_headers},
        {"flags", l_CMessage_flags},
        {"__gc", l_CMessage_destructor},
        {NULL, NULL}
    };
    luaL_newmetatable(l, "luaL_CMessage");

#if LUA_VERSION_NUM == 501
    luaL_register(l, NULL, sFooRegs);
#elif LUA_VERSION_NUM == 502
    luaL_setfuncs(l, sFooRegs, 0);
#else
#error unsupported Lua version
#endif

    lua_pushvalue(l, -1);
    lua_setfield(l, -1, "__index");
    lua_setglobal(l, "Message");
}
