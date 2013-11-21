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

-- WRITE STAT
redis.call('ZINCRBY', 'RANK_W', 1, uid)
local cn = redis.call('ZCARD', 'RANK_W')
if cn > 1000 then
   redis.call('ZREMRANGEBYRANK', 'RANK_W', 0, cn-1000)
end


return ofl
--return {tonumber(idp[1]), tonumber(idp[2])}


