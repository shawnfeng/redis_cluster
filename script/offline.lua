local uid = KEYS[1]
local sn = KEYS[2]

local kuid = 'U.'..uid
local ksn = 'S.'..uid..'.'..sn
local kc = 'C.'..uid % 100

redis.call('SREM', kuid, sn)
redis.call('DEL', ksn)
redis.call('ZREM', kc, ksn)

return {uid, sn, ksn}


