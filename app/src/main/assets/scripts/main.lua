print("Yarr! Welcome to the land of lua on android!")

local spickle

spickle = function(intable, hide_non_text, depth, visited, output_func)
    depth = depth or 0
    visited = visited or {}
    output_func = output_func or print

    if visited[intable] then
        output_func(string.rep("\t", depth) .. "<circular reference>")
        return
    end
    visited[intable] = true

    output_func(string.rep("\t", depth) .. "{")

    local indent = string.rep("\t", depth + 1)
    local sortedKeys = {}
    for k, _ in pairs(intable) do
        table.insert(sortedKeys, k)
    end
    table.sort(sortedKeys, function(a, b) return tostring(a) < tostring(b) end)

    for _, k in ipairs(sortedKeys) do
        local v = intable[k]
        local keyStr = (type(k) == "string" and k:match("^%a[%w_]*$")) and k or ("[" .. tostring(k) .. "]")
        local line

        if type(v) == "table" then
            output_func(indent .. keyStr .. " = ")
            spickle(v, hide_non_text, depth + 1, visited, output_func)
            output_func(indent .. ",")
        elseif type(v) == "string" then
            line = indent .. keyStr .. " = \"" .. v .. "\","
        elseif type(v) == "number" or type(v) == "boolean" or type(v) == "nil" then
            line = indent .. keyStr .. " = " .. tostring(v) .. ","
        elseif not hide_non_text then
            line = indent .. keyStr .. " = <" .. type(v) .. ">,"
        end

        if line then output_func(line) end
    end

    output_func(string.rep("\t", depth) .. "}")
end

--table input, hide-unsafe-values, depth, visitid, function to run on loop
spickle(_G, false, 0, {}, function(chunk)
    -- You can buffer, log, or stream this however you like
    print(chunk)
end)
