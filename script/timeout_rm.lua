local st = KEYS[1]
local idx = KEYS[2]
local cn = KEYS[3]

st = tonumber(st) - 100
local kc = 'C.'..idx


local ses = redis.call('ZRANGEBYSCORE', kc, '-inf', st, 'WITHSCORES')

for i=1, #ses, 2 do
   redis.call('DEL', ses[i])
end

for i=1, #ses, 2 do
   local ksn = ses[i]
   local idx = string.find(ksn, '%.', 3)
   local uid = string.sub(ksn, 3, idx-1)
   local sn = string.sub(ksn, idx+1)

   redis.call('SREM', 'U.'..uid, sn)
end


return redis.call('ZREMRANGEBYSCORE', kc, '-inf', st)




