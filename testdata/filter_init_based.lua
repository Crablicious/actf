function actf.init(...)
   filtertbl = {}
   for i,v in ipairs(table.pack(...)) do
      filtertbl[v] = true
   end
end

function actf.filter(ev)
   return filtertbl[ev:name()] and 0 or 1
end
