def fib_recursive(n):
    if n < 2:
        return n
    return fib_recursive(n-1) + fib_recursive(n-2)

for i in range(16):
    print(fib_recursive(i))
