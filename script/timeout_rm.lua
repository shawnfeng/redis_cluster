if #KEYS ~= 2 then
   return {err = 'error keys size '..#KEYS}
end

local st = KEYS[1]
local cn = KEYS[2]

local kc = 'TIMEOUT_CHECK'


local lcs = redis.call('ZRANGEBYSCORE', kc, '-inf', st, 'WITHSCORES')

for i=1, #lcs, 2 do
   redis.call('DEL', lcs[i])
end

for i=1, #lcs, 2 do
   local klc = lcs[i]
   local idx = string.find(klc, '%.', 3)
   local uid = string.sub(klc, 3, idx-1)
   local lc = string.sub(klc, idx+1)

   --redis.call('SREM', 'U.'..uid, lc)
   redis.call('HDEL', 'U.'..uid, lc)
end


return redis.call('ZREMRANGEBYSCORE', kc, '-inf', st)




