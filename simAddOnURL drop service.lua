sim=require'sim'

function sysCall_info()
    return {autoStart=not sim.getBoolParam(sim.boolparam_headless),menu='Misc\nURL drop service'}
end

function sysCall_addOnScriptSuspend()
    return {cmd='cleanup'}
end

function sysCall_init()
    simURLDrop=require'simURLDrop'
    simAssimp=require'simAssimp'
end

function sysCall_msg(event)
    if event.id=='dragDropEvent' then
        local ts=string.split(event.data.mimeText,'\n')
        local impFiles=''
        for _,t in ipairs(ts) do
            t=simURLDrop.rewriteURL(t)
            if string.startswith(t,'http://') or string.startswith(t,'https://') then
                local file=simURLDrop.getURL(t,simURLDrop.download_mode.file)
                if string.endswith(t,'.ttm') or string.endswith(t,'.simmodel.xml') then
                    loadModel(file)
                elseif string.endswith(t,'.ttt') or string.endswith(t,'.simscene.xml') then
                    loadScene(file..'@keepCurrent')
                elseif string.endswith(t,'.obj') or string.endswith(t,'.stl') or string.endswith(t,'.dxf') or string.endswith(t,'.ply') or string.endswith(t,'.dae') then
                    if #impFiles>0 then
                        impFiles=impFiles..';'
                    end
                    impFiles=impFiles..file
                end
            elseif string.startswith(t,'file://') then
                if sim.getInt32Param(sim.intparam_platform)==0 then
                    file=string.gsub(t,"^file:///","")
                else
                    file=string.gsub(t,"^file://","")
                end
                if string.endswith(t,'.ttm') or string.endswith(t,'.simmodel.xml') then
                    loadModel(file)
                elseif string.endswith(t,'.ttt') or string.endswith(t,'.simscene.xml') then
                    loadScene(file..'@keepCurrent')
                elseif string.endswith(t,'.obj') or string.endswith(t,'.stl') or string.endswith(t,'.dxf') or string.endswith(t,'.ply') or string.endswith(t,'.dae') then
                    if #impFiles>0 then
                        impFiles=impFiles..';'
                    end
                    impFiles=impFiles..file
                end
            end
        end
        if #impFiles>0 then
            pcall(simAssimp.importShapesDlg,impFiles)
        end
    end
end

function loadModel(f)
    local res,ret=pcall(sim.loadModel,f)
    if res then
        sim.addLog(sim.verbosity_scriptinfos, "model loaded.")
    else
        sim.addLog(sim.verbosity_scripterrors, "failed loading model: "..ret)
    end
end

function loadScene(f)
    local res,ret=pcall(sim.loadScene,f)
    if res then
        sim.addLog(sim.verbosity_scriptinfos, "scene loaded.")
    else
        sim.addLog(sim.verbosity_scripterrors, "failed loading scene: "..ret)
    end
end
