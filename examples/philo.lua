--[[
This file is a part of ACTF.

Copyright (C) 2026  Adam Wendelin <adwe live se>

ACTF is free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

ACTF is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with ACTF. If not, see
<https://www.gnu.org/licenses/>.
--]]

--[[
philo.lua is a lua filter implementation of philo.c which calculates
how long time some philosophers spend thinking and eating. If actf
(the command-line tool) is built with lua support, this filter can be
run from the project root like: `actf -x examples/philo.lua testdata/ctfs/philo`
--]]

state = {NONE=0, EATING=1, THINKING=2}

function actf.init(...)
   tidtophilo = {}
   local mt = {__index = function () return {state=state.NONE, state_begin=0, total_thinking=0, total_eating=0} end}
   setmetatable(tidtophilo, mt)
end

function actf.filter(ev)
   local evname = ev:name()
   if evname == "begin" then
      local name = ev:get("name")
      if name == "eating" then
	 local tid = ev:get("tid")
	 local p = tidtophilo[tid]
	 p.state = state.EATING
	 p.state_begin = ev:timestamp_ns()
	 tidtophilo[tid] = p
      elseif name == "thinking" then
	 local tid = ev:get("tid")
	 local p = tidtophilo[tid]
	 p.state = state.THINKING
	 p.state_begin = ev:timestamp_ns()
	 tidtophilo[tid] = p
      end
   elseif evname == "end" then
      local tid = ev:get("tid")
      local p = rawget(tidtophilo, tid)
      if p == nil then
	 return 1
      end
      local name = ev:get("name")
      if name == "eating" and p.state == state.EATING then
	 p.total_eating = p.total_eating + (ev:timestamp_ns() - p.state_begin)
      elseif name == "thinking" and p.state == state.THINKING then
	 p.total_thinking = p.total_thinking + (ev:timestamp_ns() - p.state_begin)
      end
      p.state = state.NONE
   end
   return 1
end

function actf.fini()
   for tid, philo in pairs(tidtophilo) do
      print(string.format("tid %d: thought for %d ns and ate for %d ns",
			  tid, philo.total_thinking, philo.total_eating))
   end
end
