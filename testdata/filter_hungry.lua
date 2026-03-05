function actf.filter(ev)
   return ev:name() == 'instant' and ev:payload():get('name') == 'hungry' and 0 or 1
end
