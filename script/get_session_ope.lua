local uid = KEYS[1]

local kuid = 'U.'..uid


--return redis.call('SMEMBERS', kuid)
return redis.call('HKEYS', kuid)

