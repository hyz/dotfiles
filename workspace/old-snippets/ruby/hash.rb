#!/usr/bin/ruby

hash = {1 => 'one', 2 => 'two', 3 => 'three'}

p "p hash : #{hash}"
p "hash.each"
hash.each do |e| #e is [key,value]
  p e
end

p "hash.each_pair"
hash.each_pair do |k,v| #k and v class as in hash
  p "#{k}=#{v}"
end


p "hash.each_value"
hash.each_value do |e| #e is value
  p e
end

p 'http://stackoverflow.com/questions/16159370/ruby-hash-default-value-behavior'
# One default Array without mutation

hsh = Hash.new([])

hsh[:one] += ['one']
hsh[:two] += ['two']
# This is syntactic sugar for hsh[:two] = hsh[:two] + ['two']

p hsh[:nonexistant]
# => []
# We didn't mutate the default value, it is still an empty array

p hsh
# => { :one => ['one'], :two => ['two'] }
# This time, we *did* mutate the hash.

