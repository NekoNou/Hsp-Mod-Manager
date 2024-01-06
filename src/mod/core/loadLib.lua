QuickAccess = {}

QuickAccess._index = function(env, key)
    if not rawget(env, "MOD_INFO") then
        return _G[key]
    end

    local qaVarList = rawget(env.MOD_INFO, "qaVarList")
    if qaVarList then
        if qaVarList[key] then
            local qaVar = {}
            setmetatable(qaVar, {
                __index = function (t, k)
                    if type(k) == "table" then
                        return Hsp.read(key, unpack(k))
                    else
                        return Hsp.read(key, k)
                    end
                end,
                __newindex = function (t, k, v)
                    if type(k) == "table" then
                        Hsp.write(key, v, unpack(k))
                    else
                        Hsp.write(key, v, k)
                    end
                end
            })
            return qaVar
        end
    end

    local qaFunList = rawget(env.MOD_INFO, "qaFunList")
    if qaFunList then
        if qaFunList[key] then
            return function(...)
                return Hsp.callFunction(key, ...)
            end
        end
    end

    return _G[key]
end

QuickAccess._newindex = function(env, key, value)
    if not rawget(env, "MOD_INFO") then
        rawset(env, key, value)
        return
    end

    local qaVarList = env.MOD_INFO.qaVarList
    if qaVarList then
        if qaVarList[key] then
            Hsp.write(key, value)
            return
        end
    end

    rawset(env, key, value)
end

QuickAccess.regVar = function(list)
    local env = getfenv(2)
    local map = env.MOD_INFO.qaVarList

    if not map then
        env.MOD_INFO.qaVarList = {}
        map = env.MOD_INFO.qaVarList
    end

    for index, value in ipairs(list) do
        map[value] = true
    end
end

QuickAccess.regFun = function(list)
    local env = getfenv(2)
    local map = env.MOD_INFO.qaFunList

    if not map then
        env.MOD_INFO.qaFunList = {}
        map = env.MOD_INFO.qaFunList
    end

    for index, value in ipairs(list) do
        map[value] = true
    end
end
