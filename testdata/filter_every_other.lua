function actf.init(...)
   i = 0
end

function actf.filter(ev)
   i = i + 1
   return i % 2 == 0 and 1 or 0
end
