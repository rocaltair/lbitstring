local lbs = require "lbs"

bs = lbs.new(12)
bs:nset(1, 5)

print(bs:len())

print("=========")
for i=0, bs:len()-1 do
	print(i, bs:test(i))
end

bs:nclear(2, 3)

print("=========")
for i=0, bs:len()-1 do
	print(i, bs:test(i))
end

local s = bs:dump()
print("dump len", string.len(s))

print("=========")
local nbs = lbs.load(s)
for i=0, nbs:len()-1 do
	print(i, nbs:test(i))
end

print("equal", bs:eq(nbs))
nbs:set(10)
print("equal", bs:eq(nbs))

print("=========")
bitstring = lbs.new_with_array(65, {7, 3, 2})
