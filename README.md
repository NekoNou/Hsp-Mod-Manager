# Hsp Mod Manager
一个修改过的HSP3虚拟机，可以讲Lua脚本注入Hsp程序。旨在为Hsp游戏提供跨版本的MOD功能。

# 安装
1. [下载](https://github.com/NekoNou/OpenHSP?tab=readme-ov-file)Hmm并解压
2. 编译一个特别的start.ax（因为默认不附带标签名），方法有如下两种
   - 使用参数-d编译（注意会附带工程路径）
   - [下载](https://github.com/NekoNou/OpenHSP?tab=readme-ov-file)hsp36.zip并解压，复制hmm中的hspcmp.dll到hsp36，然后编译start.ax即可
3. 复制hmm中的lua51.dll， Hsp Mod Manager.exe，mod文件夹到start.ax目录
4. 运行Hsp Mod Manager.exe

# 创建MOD
在mod里新建文件夹，命名为MOD名。在这个文件夹里新建init.lua，输入如下内容

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
- modName为mod名，最好和文件夹名一致
- requireList代表此mod依赖的mod
  - hmm会把基础mod中所有全局变量，加载到prefix中
  - 如果prefix为空，则会加载到全局环境
- fileList代表此mod的文件，hmm会一次加载
  - prefix作用和上面一样

# 注入脚本
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
- funName为要注入到的函数
- fun为要注入的脚本
  - 注入到Function和FunReturn的脚本可以获得参数
  - 可以获得的参数类型有int，double，str
- index为运行此脚本的优先级，越小越优先，默认为0
- 返回false时不会执行注入到此位置的其他脚本
  - 注入到Goto，Gosub和Function的脚本返回false时可以让hsp程序直接返回

# 读写变量
```lua
	Hsp.read(varName, ...)
	Hsp.write(varName, value, ...)

	Hsp.read("foo", 5, 10) 				--Hsp: foo(5,10)
	Hsp.write("bar", 100, 5, 10, 15) 	--Hsp: bar(5,10,15) = 100
```
- 支持读写int，double和str
- 索引没有边界检查

# 读写参数
```lua
	Hsp.readParams(paramIndex)
	Hsp.writeParams(paramIndex, value)
```
- index从1开始

# 读写系统变量
```lua
	Hsp.readSysVar(varName)
	Hsp.writeSysVar(varName, value)
```
- varName可以是cnt、stat、refstr、refdval
- 其中stat、refstr、refdval是int、str、double型的返回值

# 调用Gosub
```lua
	Hsp.callGosub(labelName)
```

# 调用函数
```lua
	Hsp.callFunction(funName, ...)
```
```
	#defcfunc FunA int prm, double prm2, str prm3
		return "hello world"

	Hsp.callFunction("FunA", 10, 10.10, "hello world")
```
- 可以传递的参数类型有int，double，str

# 快速访问变量、函数
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
	--非数组读		Hsp: varName
	varName[0]
	--非数组写		Hsp: varName = 100
	varName = 100	
	
	--一维数组读		Hsp: varName(10)
	varName[10]				
	--一维数组写		Hsp: varName(10) = 100
	varName[10]	= 100		

	--多维数组读		Hsp: varName(5, 10)
	varName[{5, 10}]		
	--多维数组写		Hsp: varName(5, 10) = 100
	varName[{5, 10}] = 100	

	--快速访问函数	Hsp: funName prm1, prm2
	funName(prm1, prm2)
```