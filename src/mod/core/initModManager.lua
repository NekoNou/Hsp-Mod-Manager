local oldprint = print
print = function(...)
    for i = 1, select("#", ...) do
        oldprint(tostring(select(i, ...)))
        oldprint('\t')
    end
    oldprint('\n')
end

local orderedInsert = function (list, index, value)
    for i, v in ipairs(list) do
        if index < v[1] then
            table.insert(list, i, {index, value})
            return
        end
    end
    table.insert(list, {index, value})
end

local INDEX_GOTO = 1
local INDEX_GOSUB = 2
local INDEX_SUBRETURN = 3
local INDEX_FUNCTION = 4
local INDEX_FUNRETRUN = 5

Hmm.list = { {}, {}, {}, {}, {} }
local hookList = Hmm.list

local curParams = nil
local noParam = {}
local onHsp = function(index)
    return function(id, params)
        local oldParams = params
        curParams = params
        for key, fun in pairs(hookList[index][id]) do
            if fun[2](unpack(params or noParam)) == false then
                curParams = oldParams
                return false
            end
        end
        curParams = oldParams
        return true
    end
end

Hmm.onGoto = onHsp(INDEX_GOTO)
Hmm.onGosub = onHsp(INDEX_GOSUB)
Hmm.onSubReturn = onHsp(INDEX_SUBRETURN)
Hmm.onFunction = onHsp(INDEX_FUNCTION)
Hmm.onFunReturn = onHsp(INDEX_FUNRETRUN)

local hookHsp = function(regFun, hookIndex, idMap)
    return function(name, fun, index)
        local id = idMap[name:lower()]

        if not id or type(fun) ~= "function" then
            error("lnvalid argument.", 1)
            return
        end

        local list = hookList[hookIndex]

        if not list[id] then
            regFun(id);
            list[id] = {}
        end

        orderedInsert(list[id], index or 0, fun)
    end
end

Hsp.hookGoto = hookHsp(Hmm.regGoto, INDEX_GOTO, Hmm.labelId)
Hsp.hookGosub = hookHsp(Hmm.regGosub, INDEX_GOSUB, Hmm.labelId)
Hsp.hookSubReturn = hookHsp(Hmm.regSubReturn, INDEX_SUBRETURN, Hmm.labelId)
Hsp.hookFunction = hookHsp(Hmm.regFunction, INDEX_FUNCTION, Hmm.funId)
Hsp.hookFunReturn = hookHsp(Hmm.regFunReturn, INDEX_FUNRETRUN, Hmm.funId)

Hsp.writeParams = function(index, value)
    Hmm.writeParams(index, value)
    if curParams then
        curParams[index] = value
    end
end