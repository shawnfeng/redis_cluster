local uid = KEYS[1]

local kuid = 'U_'..uid


return redis.call('HGETALL', kuid)

