local uid = KEYS[1]
local sn = KEYS[2]

local kuid = 'U_'..uid
local ksn = 'S_'..uid..'_'..sn

redis.call('HDEL', kuid, sn)
redis.call('DEL', ksn)

return {uid, sn, ksn}


