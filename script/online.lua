local uid = KEYS[1]
local sn = KEYS[2]
local st = KEYS[3]

local kuid = 'U_'..uid
local ksn = 'S_'..uid..'_'..sn


redis.call('HSET', kuid, sn, st)

--redis.call('HMSET', 'S_'..sn, ARGV[1], ARGV[2], ARGV[3], ARGV[4])

for i=1, #ARGV, 2 do
   redis.call('HSET', ksn, ARGV[i], ARGV[i+1])
end
--for i, v in ipairs(ARGV) do
--   redis.call('HSET', 'S_'..sn, sn, st)

--end

--return {uid, session}
return tonumber(uid)
--return {err = "err test"}
--return {ok = "OK test"}
--return KEYS

