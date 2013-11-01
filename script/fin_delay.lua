if #KEYS ~= 6 then
   return {err = 'error keys size '..#KEYS}
end

-- user id
local uid = KEYS[1]
-- logic conneciton id
local lc = KEYS[2]
-- send index
-- local sid = KEYS[3]
-- recv index
-- local rid = KEYS[4]
-- gate locate
local gate = KEYS[5]
-- delay time
local delay = KEYS[6]


local klc = 'L.'..uid..'.'..lc
local kc = 'TIMEOUT_CHECK'

local bgate = redis.call('HGET', klc, 'GATE')

if bgate == gate then
   redis.call('ZADD', kc, delay, klc)
   redis.call('HSET', klc, 'GATE', '')
   return {ok = "OK"}
else
   return nil
end

--return {tonumber(idp[1]), tonumber(idp[2])}


