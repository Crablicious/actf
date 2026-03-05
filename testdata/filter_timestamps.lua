function actf.filter(ev)
   local ts = ev:timestamp_ns()
   return (ts >= 1750742598527225863 and ts <= 1750742599127893261) and 0 or 1
end
