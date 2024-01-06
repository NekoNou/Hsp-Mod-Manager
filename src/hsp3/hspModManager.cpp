#include "Windows.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include "stack.h"
#include "hsp3code.h"
#include "hspvar_core.h"
#include "strbuf.h"

#include "hspModManager.h"
#include "win32gui/lua.hpp"
#include <algorithm>

#define strp(dsptr) &hspctx->mem_mds[dsptr]

using namespace std;

HSPCTX* hspctx;

unordered_map<unsigned short*, int> sbrToLabelId;
unordered_map<string, int> varNameToId;
unordered_map<string, int> labelNameToId;
unordered_map<string, int> funNameToId;

unordered_map<string, int> sysVarId{
	{"cnt", 0},
	{"stat", 1},
	{"refstr", 2},
	{"refdval", 3},
};

static int sysVarType[] = {
	HSPVAR_FLAG_INT,
	HSPVAR_FLAG_INT,
	HSPVAR_FLAG_STR,
	HSPVAR_FLAG_DOUBLE
};

bool* regedGoto;
bool* regedGosub;
bool* regedSubReturn;
bool* regedFunction;
bool* regedFunReturn;

bool callByLua = false;
HSPROUTINE* expandedParams;

lua_State* L;

void initModManager(unsigned char* mem_di_val, HSPCTX* ctx)
{
	AllocConsole();
	SetConsoleTitle("Hsp Mod Manager Console");
	FILE* fp = freopen("CONOUT$", "w", stdout);

	hspctx = ctx;

	initLua();
	initHsp(mem_di_val);

	if (luaL_loadfile(L, "mod//core//init.lua"))
	{
		cout << lua_tostring(L, -1) << endl;
	}
	else {
		safeCall(0, 0);
	}
}

void initLua() {
	L = luaL_newstate();
	luaL_openlibs(L);

	lua_register(L, "print", print);

	lua_newtable(L);

	lua_pushcfunction(L, regGoto);
	lua_setfield(L, -2, "regGoto");
	lua_pushcfunction(L, regGosub);
	lua_setfield(L, -2, "regGosub");
	lua_pushcfunction(L, regSubReturn);
	lua_setfield(L, -2, "regSubReturn");
	lua_pushcfunction(L, regFunction);
	lua_setfield(L, -2, "regFunction");
	lua_pushcfunction(L, regFunReturn);
	lua_setfield(L, -2, "regFunReturn");
	lua_pushcfunction(L, writeParams);
	lua_setfield(L, -2, "writeParams");

	lua_setglobal(L, "Hmm");

	lua_newtable(L);

	lua_pushcfunction(L, readVar);
	lua_setfield(L, -2, "read");
	lua_pushcfunction(L, writeVar);
	lua_setfield(L, -2, "write");
	lua_pushcfunction(L, readParams);
	lua_setfield(L, -2, "readParams");
	lua_pushcfunction(L, readSysVar);
	lua_setfield(L, -2, "readSysVar");
	lua_pushcfunction(L, writeSysVar);
	lua_setfield(L, -2, "writeSysVar");
	lua_pushcfunction(L, callGosub);
	lua_setfield(L, -2, "callGosub");
	lua_pushcfunction(L, callFunction);
	lua_setfield(L, -2, "callFunction");
	lua_pushcfunction(L, createVarParam);
	lua_setfield(L, -2, "createVarParam");

	lua_setglobal(L, "Hsp");
}

void initHsp(unsigned char* mem_di_val)
{
	lua_getglobal(L, "Hmm");

	lua_Integer id = 0;
	auto ptr = mem_di_val;

	//lua_pushstring(L, "varId");
	//lua_newtable(L);
	//lua_pushstring(L, "varName");
	//lua_newtable(L);
	while (ptr[0] != 255)
	{
		//lua_pushstring(L, strp((ptr[3] << 16) + (ptr[2] << 8) + ptr[1]));
		//lua_pushinteger(L, id);
		//lua_settable(L, -5);

		//lua_pushinteger(L, id);
		//lua_pushstring(L, strp((ptr[3] << 16) + (ptr[2] << 8) + ptr[1]));
		//lua_settable(L, -3);

		varNameToId.insert({ strp((ptr[3] << 16) + (ptr[2] << 8) + ptr[1]), id });

		ptr += 6;
		id++;
	}
	//lua_settable(L, -5);
	//lua_settable(L, -3);

	lua_pushstring(L, "labelId");
	lua_newtable(L);
	lua_pushstring(L, "labelName");
	lua_newtable(L);
	ptr++;
	while (ptr[0] != 255)
	{
		id = (ptr[5] << 8) + ptr[4];

		lua_pushstring(L, strp((ptr[3] << 16) + (ptr[2] << 8) + ptr[1]));
		lua_pushinteger(L, id);
		lua_settable(L, -5);

		lua_pushinteger(L, id);
		lua_pushstring(L, strp((ptr[3] << 16) + (ptr[2] << 8) + ptr[1]));
		lua_settable(L, -3);

		sbrToLabelId.insert({ (unsigned short*)(hspctx->mem_mcs + (hspctx->mem_ot[id])), id });
		labelNameToId.insert({ strp((ptr[3] << 16) + (ptr[2] << 8) + ptr[1]), id });

		ptr += 6;
	}
	lua_settable(L, -5);
	lua_settable(L, -3);

	lua_pushstring(L, "funId");
	lua_newtable(L);
	lua_pushstring(L, "funName");
	lua_newtable(L);
	int funNum = hspctx->hsphed->max_finfo / sizeof(STRUCTDAT);
	for (int i = 0; i < funNum; i++)
	{
		lua_pushstring(L, strp(hspctx->mem_finfo[i].nameidx));
		lua_pushinteger(L, i);
		lua_settable(L, -5);

		lua_pushinteger(L, i);
		lua_pushstring(L, strp(hspctx->mem_finfo[i].nameidx));
		lua_settable(L, -3);

		funNameToId.insert({ strp(hspctx->mem_finfo[i].nameidx), i });
	}
	lua_settable(L, -5);
	lua_settable(L, -3);

	lua_pop(L, 1);

	regedGoto = new bool[id + 1] {0};
	regedGosub = new bool[id + 1] {0};
	regedSubReturn = new bool[id + 1] {0};

	regedFunction = new bool[funNum] {0};
	regedFunReturn = new bool[funNum] {0};
}

bool onGoto(int labelID)
{
	bool proceed = true;
	if (regedGoto[labelID])
	{
		lua_getglobal(L, "Hmm");
		lua_getfield(L, -1, "onGoto");
		lua_remove(L, -2);
		lua_pushinteger(L, labelID);
		if (!safeCall(1, 1))
		{
			proceed = lua_toboolean(L, -1);
			lua_pop(L, 1);
		}
	}
	return proceed;
}

bool onGosub(int labelID)
{
	bool proceed = true;
	if (regedGosub[labelID])
	{
		lua_getglobal(L, "Hmm");
		lua_getfield(L, -1, "onGosub");
		lua_remove(L, -2);
		lua_pushinteger(L, labelID);
		if (!safeCall(1, 1))
		{
			proceed = lua_toboolean(L, -1);
			lua_pop(L, 1);
		}
	}
	return proceed;
}

void onSubReturn(int labelID)
{
	if (regedSubReturn[labelID])
	{
		lua_getglobal(L, "Hmm");
		lua_getfield(L, -1, "onSubReturn");
		lua_remove(L, -2);
		lua_pushinteger(L, labelID);
		safeCall(1, 0);
	}
}

bool onFunction(int funId)
{
	bool proceed = true;
	if (regedFunction[funId])
	{
		lua_getglobal(L, "Hmm");
		lua_getfield(L, -1, "onFunction");
		lua_remove(L, -2);
		lua_pushinteger(L, funId);

		int prmmax = hspctx->mem_finfo[funId].prmmax;
		lua_createtable(L, prmmax, 0);
		for (int i = 0; i < prmmax; i++)
		{
			lua_pushinteger(L, i + 1);
			pushParam(i);
			lua_settable(L, -3);
		}

		if (!safeCall(2, 1))
		{
			proceed = lua_toboolean(L, -1);
			lua_pop(L, 1);
		}
	}
	return proceed;
}

void onFunReturn(int funId)
{
	if (regedFunReturn[funId])
	{
		lua_getglobal(L, "Hmm");
		lua_getfield(L, -1, "onFunReturn");
		lua_remove(L, -2);
		lua_pushinteger(L, funId);

		int prmmax = hspctx->mem_finfo[funId].prmmax;
		lua_createtable(L, prmmax, 0);
		for (int i = 0; i < prmmax; i++)
		{
			lua_pushinteger(L, i + 1);
			pushParam(i);
			lua_settable(L, -3);
		}

		safeCall(2, 0);
	}
}

// LUA FUNCITON
int print(lua_State* L) {
	cout << lua_tostring(L, 1);
	return 0;
}

int regGoto(lua_State* L) {
	int id = lua_tointeger(L, 1);
	regedGoto[id] = true;
	return 0;
}

int regGosub(lua_State* L) {
	int id = lua_tointeger(L, 1);
	regedGosub[id] = true;
	return 0;

}

int regSubReturn(lua_State* L) {
	int id = lua_tointeger(L, 1);
	regedSubReturn[id] = true;
	return 0;

}

int regFunction(lua_State* L) {
	int id = lua_tointeger(L, 1);
	regedFunction[id] = true;
	return 0;

}

int regFunReturn(lua_State* L) {
	int id = lua_tointeger(L, 1);
	regedFunReturn[id] = true;
	return 0;
}

int readVar(lua_State* L) {
	int num = lua_gettop(L);

	int id = getVarId();

	if (id == -1) {
		return 0;
	}

	int offset = getArrayOffset(id, 1);
	if (offset == -1)
	{
		return 0;
	}

	PVal* p = &hspctx->mem_var[id];

	switch (p->flag) {
	case HSPVAR_FLAG_STR: {
		if (offset == 0)
		{
			lua_pushstring(L, ((char*)p->pt));
		}
		else {
			lua_pushstring(L, *(((char**)p->master) + offset));
		}
		break;
	}
	case HSPVAR_FLAG_INT:
		lua_pushinteger(L, *(((int*)p->pt) + offset));
		break;
	case HSPVAR_FLAG_DOUBLE:
		lua_pushnumber(L, *(((double*)p->pt) + offset));
		break;
	default:
		luaL_error(L, "Unsupported variable types");
		return 0;
	}

	return 1;
}

int writeVar(lua_State* L) {
	int num = lua_gettop(L);

	int id = getVarId();

	if (id == -1) {
		return 0;
	}

	int offset = getArrayOffset(id, 2);
	if (offset == -1)
	{
		return 0;
	}

	PVal* p = &hspctx->mem_var[id];

	switch (p->flag) {
	case HSPVAR_FLAG_STR: {
		HspVarProc* proc = HspVarCoreGetProc(HSPVAR_FLAG_STR);
		int oldOffset = p->offset;
		p->offset = offset;

		if (!lua_isstring(L, 2))
		{
			luaL_error(L, "incorrect type, string required");
			return 0;
		}

		proc->Set(p, proc->GetPtr(p), lua_tostring(L, 2));

		p->offset = oldOffset;
		break;
	}
	case HSPVAR_FLAG_INT:
		if (!lua_isnumber(L, 2))
		{
			luaL_error(L, "incorrect type, number required");
			return 0;
		}

		*(((int*)p->pt) + offset) = lua_tointeger(L, 2);
		break;
	case HSPVAR_FLAG_DOUBLE:
		if (!lua_isnumber(L, 2))
		{
			luaL_error(L, "incorrect type, number required");
			return 0;
		}

		*(((double*)p->pt) + offset) = lua_tonumber(L, 2);
		break;
	default:
		luaL_error(L, "unsupported variable types");
		return 0;
	}

	return 0;
}

int readParams(lua_State* L) {
	int index = getParamIndex();
	if (index == -1)
	{
		return 0;
	}

	pushParam(index);

	return 1;
}

int writeParams(lua_State* L) {
	int index = getParamIndex();
	if (index == -1)
	{
		return 0;
	}

	HSPROUTINE* r = ((HSPROUTINE*)hspctx->prmstack) - 1;
	STRUCTPRM* prm = &hspctx->mem_minfo[r->param->prmindex] + index;
	char* out = (char*)hspctx->prmstack + prm->offset;

	string str;
	char* ss;

	switch (prm->mptype) {
	case MPTYPE_INUM:
		if (!lua_isnumber(L, 2))
		{
			luaL_error(L, "incorrect argument 2: incorrect value type");
			return 0;
		}

		*(int*)out = lua_tointeger(L, 2);
		break;
	case MPTYPE_DNUM:
		if (!lua_isnumber(L, 2))
		{
			luaL_error(L, "incorrect argument 2: incorrect value type");
			return 0;
		}

		*(double*)out = lua_tonumber(L, 2);
		break;
	case MPTYPE_LOCALSTRING:
		if (!lua_isstring(L, 2))
		{
			luaL_error(L, "incorrect argument 2: incorrect value type");
			return 0;
		}

		str = lua_tostring(L, 2);

		sbFree(*(char**)out);
		ss = sbAlloc((int)strlen(str.c_str()) + 1);
		strcpy(ss, str.c_str());
		*(char**)out = ss;
		break;
	default:
		luaL_error(L, "unsupported variable types");
		return 0;
	}

	return 0;
}

int readSysVar(lua_State* L) {
	int varId = getSysVarId();
	if (varId == -1)
	{
		return 0;
	}

	void* ptr = getSysVarPtr(varId);

	if (ptr == NULL)
	{
		return 0;
	}

	switch (sysVarType[varId]) {
	case HSPVAR_FLAG_INT:
		lua_pushinteger(L, *(int*)ptr);
		break;
	case HSPVAR_FLAG_STR:
		lua_pushstring(L, *(char**)ptr);
		break;
	case HSPVAR_FLAG_DOUBLE:
		lua_pushnumber(L, *(double*)ptr);
		break;
	}

	return 1;
}

int writeSysVar(lua_State* L) {
	int varId = getSysVarId();
	if (varId == -1)
	{
		return 0;
	}

	void* ptr = getSysVarPtr(varId);

	if (ptr == NULL)
	{
		return 0;
	}

	string str;

	switch (sysVarType[varId]) {
	case HSPVAR_FLAG_INT:
		if (!lua_isnumber(L, 2))
		{
			luaL_error(L, "incorrect argument 2: incorrect value type");
			return 0;
		}

		*(int*)ptr = lua_tointeger(L, 2);
		break;
	case HSPVAR_FLAG_DOUBLE:
		if (!lua_isnumber(L, 2))
		{
			luaL_error(L, "incorrect argument 2: incorrect value type");
			return 0;
		}

		*(double*)ptr = lua_tonumber(L, 2);
		break;
	case HSPVAR_FLAG_STR:
		if (!lua_isstring(L, 2))
		{
			luaL_error(L, "incorrect argument 2: incorrect value type");
			return 0;
		}

		str = lua_tostring(L, 2);
		strcpy(*(char**)ptr, str.c_str());
		break;
	}

	return 0;
}

int callFunction(lua_State* L) {
	int id = getFunId();
	if (id == -1)
	{
		return 0;
	}

	HSPROUTINE* r = initFunParams(id);
	if (r != NULL)
	{
		expandedParams = r;
		callByLua = true;

		code_callfunc(id);
	}

	return 0;
}

int callGosub(lua_State* L) {
	int id = getLabelId();
	if (id == -1)
	{
		return 0;
	}

	cmdfunc_gosub((unsigned short*)(hspctx->mem_mcs + (hspctx->mem_ot[id])));
	return 0;
}

int createVarParam(lua_State* L) {
	int id = getVarId();

	if (id == -1) {
		return 0;
	}

	int offset = getArrayOffset(id, 1);
	if (offset == -1)
	{
		return 0;
	}

	string name = lua_tostring(L, 1);
	transform(name.begin(), name.end(), name.begin(), tolower);

	lua_createtable(L, 2, 0);

	lua_pushinteger(L, 1);
	lua_pushstring(L, name.c_str());
	lua_settable(L, -3);

	lua_pushinteger(L, 2);
	lua_pushinteger(L, offset);
	lua_settable(L, -3);

	return 1;
}

int printLuaError(lua_State* L) {
	string msg = lua_tostring(L, -1);
	luaL_traceback(L, L, msg.c_str(), 1);
	cout << lua_tostring(L, -1) << endl;
	lua_pop(L, 2);
	return 0;
}

//
int getLabelIdBySbr(unsigned short* sbr)
{
	return sbrToLabelId[sbr];
}

void pushParam(int paramIndex) {
	HSPROUTINE* r = ((HSPROUTINE*)hspctx->prmstack) - 1;
	STRUCTPRM* prm = &hspctx->mem_minfo[r->param->prmindex] + paramIndex;
	char* out = (char*)hspctx->prmstack + prm->offset;

	switch (prm->mptype) {
	case MPTYPE_INUM:
		lua_pushinteger(L, *(int*)out);
		break;
	case MPTYPE_DNUM:
		lua_pushnumber(L, *(double*)out);
		break;
	case MPTYPE_LOCALSTRING:
		lua_pushstring(L, *(char**)out);
		break;
	case MPTYPE_SINGLEVAR:
	case MPTYPE_ARRAYVAR: {
		MPVarData* var = (MPVarData*)out;
		int varId = (var->pval - hspctx->mem_var);

		lua_createtable(L, 2, 0);
		lua_pushinteger(L, 1);
		lua_pushstring(L, code_getdebug_varname(varId));
		lua_settable(L, -3);

		lua_pushinteger(L, 2);
		lua_pushinteger(L, var->aptr);
		lua_settable(L, -3);
		break;
	}
	default:
		lua_pushboolean(L, false);
	}
	prm++;
}

int getArrayOffset(int id, int offsetStart) {
	PVal* p = &hspctx->mem_var[id];
	int num = lua_gettop(L);
	int offset = 0;
	int arraymul = 1;

	for (int i = 0; i < min(num - offsetStart, 5); i++)
	{
		if (!lua_isnumber(L, i + offsetStart + 1))
		{
			luaL_error(L, ("incorrect argument: index [" + to_string((i + 1)) + "] not integer").c_str());
			return -1;
		}
		arraymul *= p->len[i];
		offset += lua_tointeger(L, i + offsetStart + 1) * arraymul;
	}

	return offset;
}

int getParamIndex()
{
	if (!lua_isnumber(L, 1))
	{
		luaL_error(L, "incorrect argument 1: parameter index not a interger");
		return -1;
	}

	int index = lua_tointeger(L, 1);
	HSPROUTINE* r = ((HSPROUTINE*)hspctx->prmstack) - 1;

	if (index >= r->param->prmmax || index < 1)
	{
		luaL_error(L, ("incorrect argument: incorrect index [" + to_string(index) + "]").c_str());
		return -1;
	}

	return index - 1;
}

int safeCall(int nargs, int nresults) {
	int msgh = -(nargs + 2);
	lua_pushcfunction(L, printLuaError);
	lua_insert(L, msgh);

	int rtv = lua_pcall(L, nargs, nresults, msgh);
	if (rtv)
	{
		lua_pop(L, 1);
	}
	else {
		msgh = -(nresults + 1);
		lua_remove(L, msgh);
	}

	return rtv;
}

int getSysVarId() {
	if (!lua_isstring(L, 1))
	{
		luaL_error(L, "incorrect argument 1: variable name not a string");
		return -1;
	}

	string name = lua_tostring(L, 1);

	if (sysVarId.count(name) == 0)
	{
		luaL_error(L, ("incorrect argument: unsupported system variable [" + name + "]").c_str());
		return -1;
	}

	return sysVarId[name];
}

void* getSysVarPtr(int varId) {
	switch (varId)
	{
	case 0:
	{
		if (hspctx->looplev == 0)
		{
			luaL_error(L, "not in loop, cannot access [cnt]");
			return NULL;
		}
		return &hspctx->mem_loop[hspctx->looplev].cnt;
	}
	case 1:
		return &hspctx->stat;
	case 2:
		return &hspctx->refstr;
	case 3:
		return &hspctx->refdval;
	default:
		return NULL;
	}
}

static int getId(const char* type, unordered_map<string, int> map) {

	string tip = type;
	if (!lua_isstring(L, 1))
	{
		luaL_error(L, ("incorrect argument 1: " + tip + " name not a string").c_str());
		return -1;
	}

	string name = lua_tostring(L, 1);
	transform(name.begin(), name.end(), name.begin(), tolower);
	if (!map.count(name))
	{
		luaL_error(L, ("incorrect argument 1: no " + tip + " [" + name + "]").c_str());
		return -1;
	}

	return map[name];
}

int getVarId()
{
	return getId("variable", varNameToId);
}

int getLabelId()
{
	return getId("label", labelNameToId);
}

int getFunId() {
	return getId("function", funNameToId);
}

HSPROUTINE* initFunParams(int id) {
	bool error = false;
	int num = lua_gettop(L);

	STRUCTDAT* st;
	HSPROUTINE* r;
	int size;
	char* p;
	char* out;
	STRUCTPRM* prm;

	st = &hspctx->mem_finfo[id];
	size = sizeof(HSPROUTINE) + st->size;
	prm = &hspctx->mem_minfo[st->prmindex];

	for (int i = 0;i < st->prmmax && !error;i++) {
		switch (prm->mptype) {
		case MPTYPE_INUM:
			if (num >= 2 + i)
			{
				if (!lua_isnumber(L, 2 + i))
				{
					luaL_error(L, ("incorrect argument " + to_string(2 + i) + ": parameter type is integer").c_str());
					error = true;
				}
			}
			break;
		case MPTYPE_SINGLEVAR:
		case MPTYPE_ARRAYVAR:
		{
			if (num >= 2 + i)
			{
				if (!lua_istable(L, 2 + i))
				{
					luaL_error(L, ("incorrect argument " + to_string(2 + i) + ": use Hsp.createVarParam to create variable type parameter").c_str());
					error = true;
					break;
				}
			}
			else {
				luaL_error(L, ("incorrect argument " + to_string(2 + i) + ": variable required").c_str());
				error = true;
			}
			break;
		}
		case MPTYPE_DNUM:
		{
			if (num >= 2 + i)
			{
				if (!lua_isnumber(L, 2 + i))
				{
					luaL_error(L, ("incorrect argument " + to_string(2 + i) + ": parameter type is double").c_str());
					error = true;
				}
			}
			break;
		}
		case MPTYPE_LOCALSTRING:
		{
			if (num >= 2 + i)
			{
				if (!lua_isstring(L, 2 + i))
				{
					luaL_error(L, ("incorrect argument " + to_string(2 + i) + ": parameter type is string").c_str());
					error = true;
				}
			}
			else {
				luaL_error(L, ("incorrect argument " + to_string(2 + i) + ": default parameter not allowed").c_str());
				error = true;
			}
			break;
		}
		}
		prm++;
	}

	if (error)
	{
		return NULL;
	}

	r = (HSPROUTINE*)StackPushSize(TYPE_EX_CUSTOMFUNC, size);
	p = (char*)(r + 1);
	prm = &hspctx->mem_minfo[st->prmindex];

	for (int i = 0;i < st->prmmax && !error;i++) {
		out = p + prm->offset;
		switch (prm->mptype) {
		case MPTYPE_INUM:
			if (num >= 2 + i)
			{
				*(int*)out = lua_tointeger(L, 2 + i);
			}
			else {
				*(int*)out = 0;
			}
			break;
		case MPTYPE_SINGLEVAR:
		case MPTYPE_ARRAYVAR:
		{
			lua_pushinteger(L, 1);
			lua_gettable(L, 2 + i);

			string name = lua_tostring(L, -1);
			transform(name.begin(), name.end(), name.begin(), tolower);
			int id = varNameToId[name];

			lua_pushinteger(L, 2);
			lua_gettable(L, 2 + i);
			int offset = lua_tointeger(L, -1);

			MPVarData* var;
			var = (MPVarData*)out;
			var->pval = &hspctx->mem_var[id];
			var->aptr = offset;
			break;
		}
		case MPTYPE_DNUM:
		{
			if (num >= 2 + i)
			{
				*(double*)out = lua_tonumber(L, 2 + i);
			}
			else {
				*(double*)out = 0;
			}
			break;
		}
		case MPTYPE_LOCALSTRING:
		{
			char* str = (char*)lua_tostring(L, 2 + i);
			char* ss;
			ss = sbAlloc((int)strlen(str) + 1);
			strcpy(ss, str);
			*(char**)out = ss;
			break;
		}
		}
		prm++;
	}

	return r;
}

bool isCallByLua() {
	return callByLua;
}

HSPROUTINE* doCallByLua() {
	callByLua = false;
	return expandedParams;
}