
local rv = {}

for i,uid in ipairs(KEYS) do

   local kuid = 'U.'..uid
   --local lcs = redis.call('SMEMBERS', kuid)
   local lcs = redis.call('HKEYS', kuid)
   for j=1, #lcs, 1 do
      local sid = {}
      local lc = lcs[j]
      sid[#sid+1] = tonumber(uid)
      sid[#sid+1] = lc
      local klc = 'L.'..uid..'.'..lc
      sid[#sid+1] = redis.call('HGETALL', klc)

      rv[#rv+1] = sid
   end

end

return rv
