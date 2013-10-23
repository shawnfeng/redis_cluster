local uid = KEYS[1]
local sn = KEYS[2]
local tp = KEYS[3]
local st = KEYS[4]


local kuid = 'U.'..uid
local ksn = 'S.'..uid..'.'..sn
local kc = 'C.'..uid % 100


redis.call('HSETNX', kuid, sn, tp)
--redis.call('SADD', kuid, sn)
--redis.call('HMSET', 'S_'..sn, ARGV[1], ARGV[2], ARGV[3], ARGV[4])

for i=1, #ARGV, 2 do
   redis.call('HSET', ksn, ARGV[i], ARGV[i+1])
end

redis.call('ZADD', kc, st, ksn)
--for i, v in ipairs(ARGV) do
--   redis.call('HSET', 'S_'..sn, sn, st)

--end

--return {uid, session}
return tonumber(uid)
--return {err = "err test"}
--return {ok = "OK test"}
--return KEYS

