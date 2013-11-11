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
--local st = tonumber(KEYS[6])
-- client type
local ct = KEYS[7]
-- client version
local cv = KEYS[8]

-- client check
-- if ct

local kuid = 'U.'..uid
local klc = 'L.'..uid..'.'..lc
local kc = 'DLY_CK'

-- expire check
local lcs = redis.call('HKEYS', kuid)
for i,v in ipairs(lcs) do
   local tmp = 'L.'..uid..'.'..v
   if 0 == redis.call('EXISTS', tmp) then
      redis.call('HDEL', kuid, v)
   end
end

-------------------------------------------

redis.call('HSETNX', kuid, lc, ct..';'..cv)

redis.call('HMSET', klc, "SID", 0, "RID", sid, "GATE", gate, "CT", ct)
redis.call('HSETNX', klc, "CV", cv)

for i=1, #ARGV, 2 do
   redis.call('HSET', klc, ARGV[i], ARGV[i+1])
end

local ct = tonumber(ct)
local ep

if ct >= 100 and ct < 200 then
   --st = '+inf'
   ep = 600

elseif ct >= 200 and ct < 300 then
   ep = 600

elseif ct >= 0 and ct < 100 then
   ep = 1200

elseif ct == 100000 then
   ep = 60

else
   ep = 600

end

redis.call('EXPIRE', klc, ep)

if redis.call('TTL', kuid) < ep then
   redis.call('EXPIRE', kuid, ep)
end

--redis.call('ZADD', kc, st, klc)
redis.call('ZREM', kc, klc)


return {0, tonumber(sid)}

--return {err = "err test"}
--return {ok = "OK test"}


