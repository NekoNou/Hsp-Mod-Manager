local loadedMod = {}
local delayLoadMod = {}
local modEnvMt = { __index = QuickAccess._index, __newindex = QuickAccess._newindex}

local loadSrc = function(path, env)
    local code, errorMsg = loadfile(path, "bt", env);
    if (code ~= nil) then
        code()
    else
        error(errorMsg, 2)
    end
end

local hasLoaded = function(modName)
    return loadedMod[modName] ~= nil
end

local delayLoad = function()
    for modName, modInfo in pairs(delayLoadMod) do
        LoadMod(modInfo)
    end
end

local doLoadMod = function(modInfo)
    for key, requireInfo in pairs(modInfo.requireList) do
        if (requireInfo.prefix == "" or requireInfo.prefix == nil) then
            for k, v in pairs(loadedMod[requireInfo.modName].env) do modInfo.env[k] = v end
        else
            modInfo.env[requireInfo.prefix] = loadedMod[requireInfo.modName].env
        end
    end

    modInfo.env.MOD_NAME = modInfo.modName
    modInfo.env.MOD_INFO = modInfo

    for index, srcInfo in ipairs(modInfo.fileList) do
        if (srcInfo.prefix == "" or srcInfo.prefix == nil) then
            loadSrc(modInfo.path .. "/" .. srcInfo.path, modInfo.env)
        else
            modInfo.env[srcInfo.prefix] = {}
            loadSrc(modInfo.path .. "/" .. srcInfo.path, modInfo.env[srcInfo.prefix])
        end
    end

    loadedMod[modInfo.modName] = modInfo
    delayLoadMod[modInfo.modName] = nil
    print("\t [" .. modInfo.modName .. "] has been loaded successfully.")

    delayLoad()
end

local clearDelayLoadMod = function()
    for modName, modInfo in pairs(delayLoadMod) do
        print("\t [" .. modName .. "] loading failed, the following mods is missing.")
        for key, requireInfo in pairs(modInfo.requireList) do
            if (not hasLoaded(requireInfo.modName)) then print("\t\t" .. requireInfo.modName) end
        end
    end

    delayLoadMod = {}
end

local loadMods = function()
    local dirinfo = io.popen("dir mod /b")
    while true do
        local modPath = dirinfo:read("*l")
        if (modPath == nil) then break end

        if modPath ~= "core" then
            local path = "mod/" .. modPath .. "/init.lua"
            local file = io.open(path)
            if file then
                local modEnv = { ["MOD_PATH"] = "mod/" .. modPath }
                setmetatable(modEnv, modEnvMt)

                load(file:read("*a"), modPath, "bt", modEnv)()
                file:close()
            end
        end
    end

    clearDelayLoadMod()
end

LoadMod = function(modInfo)
    if (hasLoaded(modInfo.modName)) then error("ERROR:  [" .. modInfo.modName .. "] has already loaded.") end

    if not modInfo.env then
        modInfo.env = getfenv(2)
        modInfo.path = modInfo.env.MOD_PATH
        modInfo.env.MOD_PATH = nil
    end

    local requireList = modInfo.requireList
    for key, requireInfo in pairs(requireList) do
        if (not hasLoaded(requireInfo.modName)) then
            delayLoadMod[modInfo.modName] = modInfo
            return
        end
    end

    doLoadMod(modInfo)
end

GetModInfo = function (modName)
    return loadedMod[modName]
end

loadMods()