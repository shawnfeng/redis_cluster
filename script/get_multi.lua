
local rv = {}

for i,uid in ipairs(KEYS) do

   local kuid = 'U_'..uid
   local ses = redis.call('HGETALL', kuid)
   for j=1, #ses, 2 do
      local sid = {}
      local sn = ses[j]
      sid[#sid+1] = tonumber(uid)
      sid[#sid+1] = sn
      local ksn = 'S_'..uid..'_'..sn
      sid[#sid+1] = redis.call('HGETALL', ksn)

      rv[#rv+1] = sid
   end

end

return rv
