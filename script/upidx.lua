if #KEYS ~= 8 then
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
local st = tonumber(KEYS[6])
-- client type
local ct = KEYS[7]
-- client version
local cv = KEYS[8]

-- client check
-- if ct

local kuid = 'U.'..uid
local klc = 'L.'..uid..'.'..lc
local kc = 'TIMEOUT_CHECK'

redis.call('HSETNX', kuid, lc, ct..';'..cv)

redis.call('HMSET', klc, "SID", 0, "RID", sid, "GATE", gate, "CT", ct)
redis.call('HSETNX', klc, "CV", cv)

for i=1, #ARGV, 2 do
   redis.call('HSET', klc, ARGV[i], ARGV[i+1])
end

local ct = tonumber(ct)

if ct >= 100 and ct < 300 then
   st = st + 600

elseif ct >= 0 and ct < 100 then
   st = st + 1200

elseif ct == 100000 then
   st = st + 20

else
   st = st + 600

end


redis.call('ZADD', kc, st, klc)

return {0, tonumber(sid)}

--return {err = "err test"}
--return {ok = "OK test"}


