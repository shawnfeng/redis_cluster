
local rv = {}

for i,uid in ipairs(KEYS) do

   local kuid = 'U.'..uid
   --local ses = redis.call('SMEMBERS', kuid)
   local ses = redis.call('HKEYS', kuid)
   for j=1, #ses, 1 do
      local sid = {}
      local sn = ses[j]
      sid[#sid+1] = tonumber(uid)
      sid[#sid+1] = sn
      local ksn = 'S.'..uid..'.'..sn
      sid[#sid+1] = redis.call('HGETALL', ksn)

      rv[#rv+1] = sid
   end

end

return rv
