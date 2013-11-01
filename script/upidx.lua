if #KEYS ~= 6 then
   return {err = 'error keys size '..#KEYS}
end

-- user id
local uid = KEYS[1]
-- logic conneciton id
local lc = KEYS[2]
-- send index
local sid = KEYS[3]
-- recv index
local rid = KEYS[4]
-- gate locate
local gate = KEYS[5]
-- expire stamp
local st = KEYS[6]


local klc = 'L.'..uid..'.'..lc
local kc = 'TIMEOUT_CHECK'

local ifo = redis.call('HMGET', klc, 'SID', 'RID', "GATE")

if ifo[1] then
   if ifo[3] ~= gate then
      redis.call('HSET', klc, "GATE", gate)
   end

   redis.call('ZADD', kc, st, klc)

   return {tonumber(ifo[1]), tonumber(ifo[2])}

else 
   return nil
end


