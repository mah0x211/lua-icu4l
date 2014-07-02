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
 *  charset.c
 *  lua-icu4l
 *
 *  Created by Masatoshi Teruya on 14/07/02.
 *
 */


#include "icu4l.h"
#include <unicode/ucsdet.h>


typedef struct {
    UCharsetDetector *ucd;
    const UCharsetMatch *match;
    int32_t nmatch;
} icu4l_charset_t;


static int detect_lua( lua_State *L )
{
    int argc = lua_gettop( L );
    icu4l_charset_t *charset = (icu4l_charset_t*)luaL_checkudata( L, 1, ICU4L_CHARSET_MT );
    size_t len = 0;
    const char *str = luaL_checklstring( L, 2, &len );
    UErrorCode ec = U_ZERO_ERROR;
    int detectAll = 0;
    size_t hlen = 0;
    const char *hint = NULL;
    
    // check argument
    if( argc > 2 )
    {
        if( !lua_isboolean( L, 3 ) ){
            return luaL_argerror( L, 3, "argument#3 must be type of boolean" );
        }
        detectAll = lua_toboolean( L, 3 );
        
        if( argc > 3 ){
            hint = luaL_checklstring( L, 4, &hlen );
        }
    }
    
    ucsdet_setText( charset->ucd, str, len, &ec );
    if( !U_FAILURE( ec ) )
    {
        int32_t nmatch = 0;
        
        if( hlen )
        {
            ucsdet_setDeclaredEncoding( charset->ucd, hint, hlen, &ec );
            if( U_FAILURE( ec ) ){
                goto RET_ERROR;
            }
        }
        
        if( detectAll ){
            charset->match = (const UCharsetMatch*)ucsdet_detectAll( 
                                charset->ucd, &charset->nmatch, &ec 
                             );
            nmatch = charset->nmatch;
        }
        else {
            charset->match = ucsdet_detect( charset->ucd, &ec );
            charset->nmatch = -1;
            nmatch = 1;
        }
        
        if( !U_FAILURE( ec ) ){
            lua_pushinteger( L, nmatch );
            return 1;
        }
    }

RET_ERROR:    
    // got error
    lua_pushnil( L );
    lua_pushstring( L, u_errorName( ec ) );
    
    return 2;
}


#define getMatchOf(L,type,getter,setter) do { \
    icu4l_charset_t *charset = (icu4l_charset_t*)luaL_checkudata( L, 1, ICU4L_CHARSET_MT ); \
    const UCharsetMatch *match = NULL; \
    UErrorCode ec = U_ZERO_ERROR; \
    type data; \
    \
    if( !charset->ucd || !charset->nmatch ){ \
        goto RET_NIL; \
    } \
    else if( charset->nmatch == -1 ){ \
        match = charset->match; \
    } \
    /* check argument */ \
    else \
    { \
        lua_Integer idx = 0; \
        if( lua_gettop( L ) > 1 ) \
        { \
            idx = luaL_checkinteger( L, 2 ); \
            if( idx < 1 || idx > charset->nmatch ){ \
                goto RET_NIL; \
            } \
            idx--; \
        } \
        \
        match = ((const UCharsetMatch**)charset->match)[idx]; \
    } \
    data = getter( match, &ec ); \
    if( !U_FAILURE( ec ) ){ \
        setter( L, data ); \
        return 1; \
    } \
    \
    /* got error */ \
    lua_pushnil( L ); \
    lua_pushstring( L, u_errorName( ec ) ); \
    return 2; \
    \
RET_NIL: \
    lua_pushnil( L ); \
    return 1; \
}while(0)

static int name_lua( lua_State *L )
{
    getMatchOf( L, const char*, ucsdet_getName, lua_pushstring );
}

static int lang_lua( lua_State *L )
{
    getMatchOf( L, const char*, ucsdet_getLanguage, lua_pushstring );
}

static int confidence_lua( lua_State *L )
{
    getMatchOf( L, int32_t, ucsdet_getConfidence, lua_pushinteger );
}


static int alloc_lua( lua_State *L )
{
    icu4l_charset_t *charset = (icu4l_charset_t*)lua_newuserdata( L, sizeof( icu4l_charset_t ) );
    const char *errstr = NULL;
    
    if( !charset ){
        errstr = strerror( errno );
    }
    else
    {
        UErrorCode status = U_ZERO_ERROR;
        
        charset->ucd = ucsdet_open( &status );
        if( U_FAILURE( status ) ){
            errstr = u_errorName( status );
        }
        else {
            charset->match = NULL;
            charset->nmatch = 0;
            // set metatable
            luaL_getmetatable( L, ICU4L_CHARSET_MT );
            lua_setmetatable( L, -2 );
            return 1;
        }
    }
    
    // got error
    lua_pushnil( L );
    lua_pushstring( L, errstr );
     
    return 2;
}


static int gc_lua( lua_State *L )
{
    icu4l_charset_t *charset = (icu4l_charset_t*)lua_touserdata( L, 1 );
    
    ucsdet_close( charset->ucd );
    
    return 0;
}

static int tostring_lua( lua_State *L )
{
    return TOSTRING_MT( L, ICU4L_CHARSET_MT );
}


LUALIB_API int luaopen_icu4l_charset( lua_State *L )
{
    struct luaL_Reg mmethod[] = {
        { "__gc", gc_lua },
        { "__tostring", tostring_lua },
        { NULL, NULL }
    };
    struct luaL_Reg method[] = {
        // method
        { "detect", detect_lua },
        { "name", name_lua },
        { "lang", lang_lua },
        { "confidence", confidence_lua },
        { NULL, NULL }
    };
    
    icu4l_define_mt( L, ICU4L_CHARSET_MT, mmethod, method );
    // add function
    lua_pushcfunction( L, alloc_lua );
    
    return 1;
}

