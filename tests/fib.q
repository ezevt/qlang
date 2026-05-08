let a = 1
let b = 1

while a < 100 do
    print(a)

    let c = a
    a = b + a
    b = c
end
