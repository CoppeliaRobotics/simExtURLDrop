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
--            if string.startswith(t,'http://') or string.startswith(t,'https://') then
                if string.endswith(t,'.ttm') or string.endswith(t,'.simmodel.xml') then
                    local data=simURLDrop.getURL(t)
                    sim.loadModel(data)
                elseif string.endswith(t,'.ttt') or string.endswith(t,'.simscene.xml') then
                    local data=simURLDrop.getURL(t)
                    sim.loadScene(data..'@keepCurrent')
                elseif string.endswith(t,'.obj') or string.endswith(t,'.stl') or string.endswith(t,'.dxf') or string.endswith(t,'.ply') or string.endswith(t,'.dae') then
                    if #impFiles>0 then
                        impFiles=impFiles..';'
                    end
                    if sim.getInt32Param(sim.intparam_platform)==0 then
                        impFiles=impFiles..string.gsub(t,"^file:///","")
                    else
                        impFiles=impFiles..string.gsub(t,"^file://","")
                    end
                end
--            end
        end
        if #impFiles>0 then
            simAssimp.importShapesDlg(impFiles)
        end
    end
end
