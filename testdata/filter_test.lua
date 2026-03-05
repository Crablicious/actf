function actf.filter(ev)
   if ev:get('name') ~= ev:payload():get('name') then
      print(string.format(
	       "ev:get('name')='%s' but should match ev:payload():get('name')='%s'",
	       ev:get('name'), ev:payload():get('name')))
      return -1
   elseif ev:get('specific_context') ~= nil then
      print("ev:get('specific_context') should be nil")
      return -1
   elseif ev:name() == nil then
      print("ev:name() should not be nil")
      return -1
   elseif ev:namespace() ~= nil then
      print("ev:namespace() should be nil")
      return -1
   elseif ev:uid() ~= nil then
      print("ev:uid() should be nil")
      return -1
   elseif ev:attributes() == nil then
      print("ev:attributes() should not be nil")
      return -1
   elseif ev:name() == 'begin' and ev:attributes():rawget('tarc'):get('type') ~= 'duration:begin' then
      print("ev:attributes():get('tarc'):get('type') for 'begin' event should be 'duration:begin'")
      return -1
   elseif ev:get('nonexistent') ~= nil then
      print("ev:get('nonexistent') should be nil")
      return -1
   elseif ev:extensions() ~= nil then
      print("ev:extensions() should be nil")
      return -1
   elseif ev:packet() == nil then
      print("ev:packet() should not be nil")
      return -1
   elseif ev:packet():header():get('magic') ~= 0xc1fc1fc1 then
      print("ev:packet():header():get('magic') should be 0xc1fc1fc1")
      return -1
   elseif ev:packet():context():get('magic') ~= nil then
      print("ev:packet():context():get('magic') should be nil")
      return -1
   elseif ev:packet():rawget('seq_nr') == nil then
      print("ev:packet():rawget('seq_nr') should not be nil")
      return -1
   elseif ev:packet():context():rawget('seq_nr'):value() ~= ev:packet():context():get('seq_nr') then
      print("ev:packet():context():rawget('seq_nr'):value() should be equal to ev:packet():context():get('seq_nr')")
      return -1
   elseif ev:packet():discard_snapshot() ~= 0 then
      print("ev:packet():discard_snapshot() should be zero")
      return -1
   elseif #ev:packet():context() ~= 6 then
      print(string.format('ev:packet():context().len()=%d but should be 6', ev:packet():context().len()))
      return -1
   elseif ev:packet():context():get(6) == nil then -- last element of packet context
      print('ev:packet():context():get(6) should not be nil')
      return -1
   elseif ev:packet():context():get(7) ~= nil then -- one past last element of packet context
      print('ev:packet():context():get(7) should be nil')
      return -1
   end
   return 0
end
