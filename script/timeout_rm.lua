if #KEYS ~= 2 then
   return {err = 'error keys size '..#KEYS}
end

local st = KEYS[1]
local cn = KEYS[2]

local kc = 'DLY_CK'


local lcs = redis.call('ZRANGEBYSCORE', kc, '-inf', st, 'WITHSCORES')

for i=1, #lcs, 2 do
   redis.call('DEL', lcs[i])
end

local ofl = {}
for i=1, #lcs, 2 do
   local klc = lcs[i]
   local idx = string.find(klc, '%.', 3)
   local uid = string.sub(klc, 3, idx-1)
   local lc = string.sub(klc, idx+1)
   local kuid = 'U.'..uid

   ofl[#ofl+1] = tonumber(uid)
   ofl[#ofl+1] = redis.call('HGET', kuid, lc)

   redis.call('HDEL', kuid, lc)
end


redis.call('ZREMRANGEBYSCORE', kc, '-inf', st)

return ofl




