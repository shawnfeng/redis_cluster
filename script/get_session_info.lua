local uid = KEYS[1]
local sn = KEYS[2]


local kuid = 'U_'..uid
local ksn = 'S_'..uid..'_'..sn

return redis.call('HGETALL', ksn)

--for i=1, #ARGV, 2 do
--   redis.call('HSET', ksn, ARGV[i], ARGV[i+1])
--end
