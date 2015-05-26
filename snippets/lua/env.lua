function pp(env)
    for x,y in pairs(env) do
        print(x,tostring(y))
    end
end

myvar="global"

print(_ENV)

function main()
    local print = print
    local tostring = tostring

    local myvar = "main"

    function f()
        print(myvar)
        print(_ENV)
        local _ENV = { myvar="_ENV" }
        print(_ENV)
        function g()
            print(_ENV)
            print(myvar)
        end
        g()
    end
    f()
end

main()

