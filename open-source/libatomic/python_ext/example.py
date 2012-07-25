import atomic

a = atomic.new()

print a

atomic.inc(a)

print a

atomic.dec(a)

print a