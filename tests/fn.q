fn makeCounter()
    let count = 0

    fn counter()
        count = count + 1
        print(count)
    end

    ret counter
end

let c = makeCounter()
let b = makeCounter()

c()
c()
c()
c()

b()
b()


let i = 0
while i < 10 do
    print(i)
    if i > 5 then
        break
    end
    i = i+1
end
