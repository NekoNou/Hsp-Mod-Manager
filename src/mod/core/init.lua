loadfile("mod/core/initModManager.lua", "bt")()
loadfile("mod/core/loadLib.lua", "bt")()

print("loading mods ...")
loadfile("mod/core/loadMods.lua", "bt")()
print("loading complete")