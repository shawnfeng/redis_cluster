
local rv = {}

for i,uid in ipairs(KEYS) do
   local sid = {}

   local kuid = 'U_'..uid
   local ses = redis.call('HGETALL', kuid)
   for j=1, #ses, 2 do
      local sn = ses[j]
      sid[#sid+1] = uid
      sid[#sid+1] = sn
      local ksn = 'S_'..uid..'_'..sn
      sid[#sid+1] = redis.call('HGETALL', ksn)
   end
   rv[#rv+1] = sid
end

return rv
