
fn fib(n)
    if n < 2 then
        ret n
    end

    ret fib(n-1) + fib(n-2)
end

let i = 1

while i <= 15 do
    print(fib(i))
    i = i + 1
end
