#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <lua.h>
#include <lauxlib.h>

#include "bitstring.h"

static const union {
        char c;
        int i;
} ENDIAN_DATA = {1};

#define IS_LITTLE_ENDIAN (ENDIAN_DATA.i)

#if LUA_VERSION_NUM < 502
#  define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#endif

#define BS_CLS "cls{bitstring}"

#define CHECK_BITSTRING(L, idx)\
	(*(lbitstring_t **) luaL_checkudata(L, idx, BS_CLS))

#define LUA_BIND_META(L, type_t, ptr, mname) do {                   \
	type_t **my__p = lua_newuserdata(L, sizeof(void *));        \
	*my__p = ptr;                                               \
	luaL_getmetatable(L, mname);                                \
	lua_setmetatable(L, -2);                                    \
} while(0)

#define bitstring_size(nbits) \
	(unsigned int)bitstr_size(nbits) * sizeof(bitstr_t) + sizeof(size_t)

#define	bitstring_alloc(nbits) \
	(lbitstring_t *)calloc(1, bitstring_size(nbits))

#define BS_CHECK_ARRAY_LEN(L, idx, rqlen) do {                                                             \
	size_t len;                                                                                        \
	int isarray = luac__bs_is_array(L, 2);                                                             \
	if (isarray) {                                                                                     \
	        len = lua_objlen(L, 2);                                                                    \
	}                                                                                                  \
	if (!isarray || len != rqlen) {                                                                    \
	        return luaL_error(L, "#%d array(len=%d) required in %s", idx, rqlen, __FUNCTION__);        \
	}                                                                                                  \
} while(0)

static uint32_t uint32tolittle(uint32_t d)
{
	unsigned char tmp;
	unsigned char b[sizeof(uint32_t)];
	memcpy(&b, &d, sizeof(d));
	if (IS_LITTLE_ENDIAN) {
		return d;
	}
	tmp = b[0];
	b[0] = b[3];
	b[3] = tmp;

	tmp = b[1];
	b[1] = b[2];
	b[2] = tmp;
	return *(uint32_t *)(&b);
}

typedef struct lbitstring_s {
	size_t len;
	bitstr_t bitstring[0];
} lbitstring_t;

static int luac__bs_is_array(lua_State *L, int idx) 
{
	int isarray = 0; 
	int len = 0; 
	int top = lua_gettop(L);
	lua_pushvalue(L, idx);
	if (!lua_istable(L, -1)) 
		goto finished;

	len = lua_objlen(L, -1); 
	if (len > 0) { 
		lua_pushnumber(L, len);
		if (lua_next(L,-2) == 0) { 
			isarray = 1; 
		}    
	}    
finished:
	lua_settop(L, top);
	return isarray;
}

static int lua__bs_new(lua_State *L)
{
	size_t len = luaL_checkinteger(L, 1);
	lbitstring_t *bs = bitstring_alloc(len);
	bs->len = len;
	LUA_BIND_META(L, lbitstring_t, bs, BS_CLS);
	return 1;
}

static int lua__bs_new_with_array(lua_State *L)
{
	lbitstring_t *bs = NULL;
	int top;
	size_t i = 0;
	size_t bitlen = luaL_checkinteger(L, 1);
	size_t arraylen = bitlen % (sizeof(int32_t) * 8) == 0 
		? (bitlen / (sizeof(int32_t) * 8)) : (bitlen / (sizeof(int32_t) * 8) + 1);
	BS_CHECK_ARRAY_LEN(L, 2, arraylen);
	top = lua_gettop(L);
	bs = bitstring_alloc(bitlen);
	bs->len = bitlen;

	lua_pushnil(L);
	while (lua_next(L, 2) != 0) {
		uint32_t v = (uint32_t)lua_tointeger(L, -1);
		v = uint32tolittle(v);
		size_t left = bitlen - i * sizeof(uint32_t);
		lua_pop(L, 1);
		memcpy((void *)&bs->bitstring[i * sizeof(uint32_t)], &v, left%sizeof(uint32_t));
		i++;
	}
	lua_settop(L, top);
	LUA_BIND_META(L, lbitstring_t, bs, BS_CLS);
	return 1;
}

static int lua__bs_gc(lua_State *L)
{
	lbitstring_t *bs = CHECK_BITSTRING(L, 1);
	free(bs);
	return 0;
}

static int lua__bs_len(lua_State *L)
{
	lbitstring_t *bs = CHECK_BITSTRING(L, 1);
	lua_pushinteger(L, (lua_Integer)bs->len);
	return 1;
}


static int lua__bs_test(lua_State *L)
{
	unsigned char ret;
	lbitstring_t *bs = CHECK_BITSTRING(L, 1);
	size_t bit = luaL_checkinteger(L, 2);

	luaL_argcheck(L, bit < bs->len && bit >= 0, 2, "len error");

	ret = bit_test(bs->bitstring, bit);
	lua_pushboolean(L, ret);
	return 1;
}

static int lua__bs_set(lua_State *L)
{
	lbitstring_t *bs = CHECK_BITSTRING(L, 1);
	size_t bit = luaL_checkinteger(L, 2);

	luaL_argcheck(L, bit < bs->len && bit >= 0, 2, "len error");
	bit_set(bs->bitstring, bit);
	return 0;
}

static int lua__bs_clear(lua_State *L)
{
	lbitstring_t *bs = CHECK_BITSTRING(L, 1);
	size_t bit = luaL_checkinteger(L, 2);
	luaL_argcheck(L, bit < bs->len && bit >= 0, 2, "len error");
	bit_clear(bs->bitstring, bit);
	return 0;
}

static int lua__bs_nclear(lua_State * L)
{
	lbitstring_t *bs = CHECK_BITSTRING(L, 1);
	size_t start = luaL_checkinteger(L, 2);
	size_t stop = luaL_checkinteger(L, 3);
	luaL_argcheck(L, start < bs->len && start >= 0, 2, "len error");
	luaL_argcheck(L, stop < bs->len && stop >= 0 && stop >= start, 3, "len error");
	bit_nclear(bs->bitstring, start, stop);
	return 0;
}

static int lua__bs_nset(lua_State *L)
{
	lbitstring_t *bs = CHECK_BITSTRING(L, 1);
	size_t start = luaL_checkinteger(L, 2);
	size_t stop = luaL_checkinteger(L, 3);
	luaL_argcheck(L, start < bs->len && start >= 0, 2, "len error");
	luaL_argcheck(L, stop < bs->len && stop >= 0 && stop >= start, 3, "len error");
	bit_nset(bs->bitstring, start, stop);
	return 0;
}

static int lua__bs_ffc(lua_State *L)
{
	int ret;
	lbitstring_t *bs = CHECK_BITSTRING(L, 1);
	bit_ffc(bs->bitstring, bs->len, &ret);
	if (ret >= 0) {
		lua_pushinteger(L, ret);
		return 1;
	}
	return 0;
}

static int lua__bs_ffs(lua_State *L)
{
	int ret;
	lbitstring_t *bs = CHECK_BITSTRING(L, 1);
	bit_ffs(bs->bitstring, bs->len, &ret);
	if (ret >= 0) {
		lua_pushinteger(L, ret);
		return 1;
	}
	return 0;
}

static int lua__bs_eq(lua_State *L)
{
	int ret = 1;
	lbitstring_t *self = CHECK_BITSTRING(L, 1);
	lbitstring_t *other = CHECK_BITSTRING(L, 2);
	if (self->len != other->len 
		|| memcmp(self->bitstring, other->bitstring, bitstr_size(self->len)) != 0) {
		ret = 0;
	}
	lua_pushboolean(L, ret);
	return 1;
}

static int lua__bs_dump(lua_State *L)
{
	lbitstring_t *bs = CHECK_BITSTRING(L, 1);
	lua_pushlstring(L, (const char *)bs, bitstring_size(bs->len));
	return 1;
}

static int lua__bs_load(lua_State *L)
{
	size_t sz;
	size_t len;
	lbitstring_t *bs = NULL;
	const char *bin = luaL_checklstring(L, 1, &sz);
	len = *(size_t *)bin;
	luaL_argcheck(L, bitstring_size(len) == sz, 1, "data error!");
	/*
	 * fprintf(stderr, "bs_load,len=%lu,sz=%lu,bslen=%lu\n", len, sz, bitstring_size(len));
	 */
	bs = bitstring_alloc(len);
	bs->len = len;
	memcpy(bs->bitstring, ((lbitstring_t *)bin)->bitstring, bitstr_size(len));
	LUA_BIND_META(L, lbitstring_t, bs, BS_CLS);
	return 1;
}

static int opencls_bitstring(lua_State *L)
{
	luaL_Reg lmethods[] = {
		{"len", lua__bs_len},
		{"test", lua__bs_test},
		{"set", lua__bs_set},
		{"clear", lua__bs_clear},
		{"nset", lua__bs_nset},
		{"nclear", lua__bs_nclear},
		{"ffs", lua__bs_ffs},
		{"ffc", lua__bs_ffc},
		{"eq", lua__bs_eq},
		{"dump", lua__bs_dump},
		{NULL, NULL},
	};
	luaL_newmetatable(L, BS_CLS);
	lua_newtable(L);
	luaL_register(L, NULL, lmethods);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction (L, lua__bs_gc);
	lua_setfield (L, -2, "__gc");
	return 1;
}

int luaopen_lbs(lua_State* L)
{
	luaL_Reg lfuncs[] = {
		{"new", lua__bs_new},
		{"load", lua__bs_load},
		{"new_with_array", lua__bs_new_with_array},
		{NULL, NULL},
	};
	opencls_bitstring(L);
	luaL_newlib(L, lfuncs);
	return 1;
}
