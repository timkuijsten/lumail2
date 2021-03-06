/*
 * config_lua.cc - Export our configuration-object to Lua.
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

#include "config.h"



/**
 * @file config_lua.cc
 *
 * This file implements the trivial exporting of our CConfig class,
 * implemented in C++, to Lua.  Lua-usage looks something like this:
 *
 *<code>
 *   -- Set the outgoing email address<br/>
 *   Config:set( "global.from", "Steve Kemp <steve@example.com>" )
 *</code>
 *
 */


/**
 * Implementation of `Config:get`
 */
int l_Config_get(lua_State * l)
{
    /*
     * The key to get.
     */
    const char *name = luaL_checkstring(l, 2);

    /*
     * Get the entry - and test it has a value.
     */
    CConfig *foo = CConfig::instance();
    CConfigEntry *x = foo->get(name);

    if (x == NULL)
    {
        lua_pushnil(l);
        return 1;
    }

    /*
     * Does this configuration value hold a string?
     */
    if (x->type == CONFIG_STRING)
    {
        lua_pushstring(l, x->value.str->c_str());
        return 1;
    }
    else if (x->type == CONFIG_INTEGER)
    {
        /*
         * Does this configuration value hold an integer?
         */
        lua_pushnumber(l, *x->value.value);
        return 1;
    }
    else if (x->type == CONFIG_ARRAY)
    {
        /*
         * Does this configuration value hold an array (read: table)?
         */
        lua_newtable(l);

        int i = 1;

        for (auto it = x->value.array->begin(); it != x->value.array->end();
                ++it)
        {
            std::string value = (*it);

            lua_pushinteger(l, i);
            lua_pushstring(l, value.c_str());

            lua_settable(l, -3);

            i += 1;
        }

        return 1;
    }
    else
    {
        throw ("Invalid get-type");
        return 0;
    }
}


/**
 * Implementation of `Config:keys`
 */
int l_Config_keys(lua_State * l)
{
    /*
     * Get the keys.
     */
    CConfig *foo = CConfig::instance();
    std::vector < std::string > keys = foo->keys();

    lua_newtable(l);

    int i = 1;

    for (auto it = keys.begin(); it != keys.end(); ++it)
    {
        std::string name = (*it);

        lua_pushinteger(l, i);
        lua_pushstring(l, name.c_str());

        lua_settable(l, -3);

        i += 1;
    }

    return 1;
}


/**
 * Implementation of `Config:set`
 */
int l_Config_set(lua_State * l)
{
    /*
     * Get the key to set, and the helper.
     */
    CConfig *foo = CConfig::instance();
    const char *name = luaL_checkstring(l, 2);

    if ((lua_gettop(l) != 3) || (lua_isnil(l, 3)))
    {
        foo->delete_key(name);
        return 0;
    }

    if (lua_istable(l, 3))
    {
        std::vector < std::string > vals;

        lua_pushnil(l);

        while (lua_next(l, -2))
        {
            const char *entry = lua_tostring(l, -1);
            vals.push_back(entry);
            lua_pop(l, 1);
        }

        foo->set(name, vals);
    }
    else if (lua_isnumber(l, 3))
    {
        int value = luaL_checkinteger(l, 3);
        foo->set(name, value);
    }
    else if (lua_isstring(l, 3))
    {
        const char *value = luaL_checkstring(l, 3);
        foo->set(name, value);
    }
    else if (lua_isboolean(l, 3))
    {
        bool val = lua_toboolean(l, 3);

        if (val)
            foo->set(name, 1);
        else
            foo->set(name, 0);

    }
    else
    {
        throw ("Invalid set-type");
    }

    return 0;
}


/**
 * Register the global `Config` object to the Lua environment, and
 * setup our public methods upon which the user may operate.
 */
void InitConfig(lua_State * l)
{
    luaL_Reg sFooRegs[] =
    {
        {"get", l_Config_get},
        {"keys", l_Config_keys},
        {"set", l_Config_set},
        {NULL, NULL}
    };
    luaL_newmetatable(l, "luaL_CConfig");

#if LUA_VERSION_NUM == 501
    luaL_register(l, NULL, sFooRegs);
#elif LUA_VERSION_NUM == 502 || LUA_VERSION_NUM == 503
    luaL_setfuncs(l, sFooRegs, 0);
#else
#error We are only tested under Lua 5.1, 5.2, or 5.3.
#endif

    lua_pushvalue(l, -1);
    lua_setfield(l, -1, "__index");
    lua_setglobal(l, "Config");

}
