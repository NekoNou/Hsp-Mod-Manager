#pragma once
#include "hsp3struct.h"
#include "win32gui/lua.hpp"
#include <string>
void initModManager(unsigned char* mem_di_val, HSPCTX* hspctx);

void initLua();

void initHsp(unsigned char* mem_di_val);

bool onGoto(int labelID);

bool onGosub(int labelID);

void onSubReturn(int labelID);

bool onFunction(int funId);

void onFunReturn(int funId);

int print(lua_State* L);

int regGoto(lua_State* L);

int regGosub(lua_State* L);

int regSubReturn(lua_State* L);

int regFunction(lua_State* L);

int regFunReturn(lua_State* L);

int readVar(lua_State* L);

int writeVar(lua_State* L);

int readParams(lua_State* L);

int writeParams(lua_State* L);

int readSysVar(lua_State* L);

int writeSysVar(lua_State* L);

int callFunction(lua_State* L);

int callGosub(lua_State* L);

int createVarParam(lua_State* L);

int printLuaError(lua_State* L);

int getLabelIdBySbr(unsigned short* sbr);

void pushParam(int paramIndex);

int getArrayOffset(int id, int offsetStart);

int getParamIndex();

int safeCall(int nargs, int nresults);

int getSysVarId();

void* getSysVarPtr(int varId);

int getVarId();

int getLabelId();

int getFunId();

HSPROUTINE* initFunParams(int id);

bool isCallByLua();

HSPROUTINE* doCallByLua();
