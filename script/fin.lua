if #KEYS ~= 5 then
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


local kuid = 'U.'..uid
local klc = 'L.'..uid..'.'..lc
local kc = 'DLY_CK'

--local idp = redis.call('HMGET', klc, 'SID', 'RID')
-- don't need care the uid expire
local ofl = redis.call('HGET', kuid, lc)
redis.call('HDEL', kuid, lc)
redis.call('DEL', klc)
redis.call('ZREM', kc, klc)


return ofl
--return {tonumber(idp[1]), tonumber(idp[2])}


