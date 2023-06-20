sim=require'sim'

function sysCall_info()
    return {autoStart=not sim.getBoolParam(sim.boolparam_headless),menu='Misc\nURL drop service'}
end

function sysCall_addOnScriptSuspend()
    return {cmd='cleanup'}
end

function sysCall_init()
    simURLDrop=require'simURLDrop'
end

function sysCall_msg(event)
    if event.id=='dragDropEvent' then
        local ts=string.split(event.data.mimeText,'\n')
        for _,t in ipairs(ts) do
            t=simURLDrop.rewriteURL(t)
            if string.startswith(t,'http://') or string.startswith(t,'https://') then
                if string.endswith(t,'.ttm') or string.endswith(t,'.simmodel.xml') then
                    local data=simURLDrop.getURL(t)
                    sim.loadModel(data)
                elseif string.endswith(t,'.ttt') or string.endswith(t,'.simscene.xml') then
                    local data=simURLDrop.getURL(t)
                    sim.loadScene(data)
                end
            end
        end
    end
end
