This document was translated by deepl. The original [Chinese version](README_ZH.md).
# Hsp Mod Manager
A modified HSP3 VM that injects Lua scripts into Hsp programs. Designed to provide cross version modding capabilities for Hsp games.

# Install
1. [Download](https://github.com/NekoNou/OpenHSP?tab=readme-ov-file) Hmm and unzip
2. Compile a special start.ax (since it doesn't come with label name by default) in one of the following two ways
   - Compile with the -d parameter (note that it will be accompanied by the project path)
   - [Download](https://github.com/NekoNou/OpenHSP?tab=readme-ov-file) hsp36.zip and unzip it, copy hspcmp.dll from hmm to hsp36, and then compile start.ax
3. Copy lua51.dll, Hsp Mod Manager.exe, and the mod folder from hmm to the start.ax directory.
4. Run Hsp Mod Manager.exe

# Creating Mods
Create a new folder in the mod and name it mod name. In this folder create a new init.lua and enter the following

```lua
	LoadMod({
		modName     = "example",
    	requireList = {
        	--{modName = "base mod", prefix = ""},
    	},
    	fileList    = {
        	--{ path = "example.lua", prefix = "" },
    	},
	})
```
- modName is the name of the mod, preferably the same as the folder name.
- requireList represents the mods that this mod depends on.
  - hmm will load all global variables in the base mod into prefix
  - If prefix is empty, it will be loaded into the global environment of this mod
- fileList represents the files for this mod, hmm will load them in order after all dependencies are loaded.

# Injecting Scripts
```
	bool Hsp.hookGoto(funName, fun, index)
	bool Hsp.hookGosub(funName, fun, index)
	bool Hsp.hookSubReturn(funName, fun, index)
	bool Hsp.hookFunction(funName, fun, index)
	bool Hsp.hookFunReturn(funName, fun, index)
```
```
	#defcfunc FunA int prm, double prm2, str prm3
		return "hello world"

	Hsp.hookFunReturn("FunA", function(prm1, prm2, prm3)

	end)
```
- funName is the function to be injected into
- fun is the script to be injected
  - Scripts injected into Function and FunReturn can get parameters
  - The types of parameters that can be obtained are int, double, str
- index is the priority of running this script, the smaller the priority, the default is 0
- Returning false will not execute other scripts injected into this location
  - Scripts injected into Goto, Gosub, and Function that return false can have the hsp program directly return

# Read and write variables
```lua
	Hsp.read(varName, ...)
	Hsp.write(varName, value, ...)

	Hsp.read("foo", 5, 10) 				--Hsp: foo(5,10)
	Hsp.write("bar", 100, 5, 10, 15) 	--Hsp: bar(5,10,15) = 100
```
- Supports reading and writing int, double and str.
- Indexing without boundary checking

# Read and write parameters
```lua
	Hsp.readParams(paramIndex)
	Hsp.writeParams(paramIndex, value)
```
- index starts at 1

# Read and write system variables
```lua
	Hsp.readSysVar(varName)
	Hsp.writeSysVar(varName, value)
```
- varName can be cnt, stat, refstr, refdval
- where stat, refstr, refdval are return values of type int, str, double

# Call gosub
```lua
	Hsp.callGosub(labelName)
```

# Call function
```lua
	Hsp.callFunction(funName, ...)
```
```
	#defcfunc FunA int prm, double prm2, str prm3
		return "hello world"

	Hsp.callFunction("FunA", 10, 10.10, "hello world")
```
- The types of parameters that can be passed are int, double, str

# Quick access to variables, functions
```lua
	QuickAccess.regVar({
		varName1,
		varName2
	})

	QuickAccess.regFun({
		funName1,
		funName2
	})
```
```lua
	--non-array read			Hsp: varName
	varName[0]
	--non-array write			Hsp: varName = 100
	varName = 100	
	
	--One-dimensional array read		Hsp: varName(10)
	varName[10]				
	--One-dimensional array write		Hsp: varName(10) = 100
	varName[10] = 100		

	--multidimensional array read		Hsp: varName(5, 10)
	varName[{5, 10}]		
	--Multi-dimensional arrays write	Hsp: varName(5, 10) = 100
	varName[{5, 10}] = 100	

	--Quick Access Functions		Hsp: funName prm1, prm2
	funName(prm1, prm2)
```