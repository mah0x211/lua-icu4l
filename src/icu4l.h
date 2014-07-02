/*
 *  Copyright (C) 2014 Masatoshi Teruya
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 *
 *
 *  icu4l.h
 *  lua-icu4l
 *
 *  Created by Masatoshi Teruya on 14/07/02.
 *
 */

#ifndef ___ICU4L_H___
#define ___ICU4L_H___

#include <errno.h>
#include <string.h>
#include <lauxlib.h>

// helper macros
#define lstate_fn2tbl(L,k,v) do{ \
    lua_pushstring(L,k); \
    lua_pushcfunction(L,v); \
    lua_rawset(L,-3); \
}while(0)

// define module names
#define ICU4L_CHARSET_MT    "icu4l.charset"

// define prototypes
LUALIB_API int luaopen_icu4l_charset( lua_State *L );

// common metamethods
#define TOSTRING_MT(L,tname) ({ \
    lua_pushfstring( L, tname ": %p", lua_touserdata( L, 1 ) ); \
    1; \
})


// module definition register
static inline void icu4l_define_mt( lua_State *L, const char *tname, 
                                    struct luaL_Reg mmethod[], 
                                    struct luaL_Reg method[] )
{
    int i = 0;
    
    // create table __metatable
    luaL_newmetatable( L, tname );
    // metamethods
    while( mmethod[i].name ){
        lstate_fn2tbl( L, mmethod[i].name, mmethod[i].func );
        i++;
    }
    // methods
    lua_pushstring( L, "__index" );
    lua_newtable( L );
    i = 0;
    while( method[i].name ){
        lstate_fn2tbl( L, method[i].name, method[i].func );
        i++;
    }
    lua_rawset( L, -3 );
    lua_pop( L, 1 );
}


#endif
