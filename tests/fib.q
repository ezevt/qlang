let a = 1
let b = 1

while a < 1000 do
    print(a)

    let c = a
    a = b + a
    b = c
end
