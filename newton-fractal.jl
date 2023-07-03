#!/usr/bin/env julia

struct Poly{T}
    terms::Array{T}
end

Poly(P::Poly)::Poly = Poly(P.terms[1:deg(P)+1])
Poly(a::Number)::Poly = Poly([a])

X = Poly([0, 1])

Base.convert(::Type{Poly}, a::Number) = Poly([a])

deg(P::Number) = if P == 0; -1 else 0 end
function deg(P::Poly)
    d = length(P.terms) - 1
    while d >= 0 && P.terms[d+1] == 0
        d -= 1
    end
    d
end

Base.:+(P::Poly) = P
Base.:+(P::Number, Q::Poly) = Poly(Poly(P) + Q)
Base.:+(P::Poly, Q::Number) = Poly(P + Poly(Q))
function Base.:+(P::Poly{T}, Q::Poly{U}) where {T,U}
    lP, lQ = length(P.terms), length(Q.terms)
    l = max(lP, lQ)
    Poly(Poly([P.terms; zeros(T, l - lP)] + [Q.terms; zeros(U, l - lQ)]))
end

Base.:-(P::Poly) = Poly(-P.terms)
Base.:-(P::Poly, Q::Poly) = P + (-Q)
Base.:-(a::Number, P::Poly) = a + (-P)
Base.:-(P::Poly, a::Number) = P + (-a)

Base.:*(a::Number, P::Poly) = Poly(Poly(a*P.terms))
Base.:*(P::Poly, a::Number) = Poly(Poly(a*P.terms))
function Base.:*(P::Poly, Q::Poly)
    dP, dQ = deg(P), deg(Q)
    d = dP + dQ
    Poly(Poly([ sum( P.terms[1+k]*Q.terms[1+n-k] for k ∈ max(0,n-dQ):min(dP,n) ) for n ∈ 0:d ]))
end

function Base.:^(P::Poly, n::Integer)
    if n == 0
        Poly(1)
    elseif n == 1
        P
    else
        q,r = divrem(n, 2)
        Q = P^q
        if r == 0; Q*Q else P*Q*Q end
    end
end

(P::Poly{T})(x::T) where T = sum( P.terms[k]*x^(k-1) for k ∈ 1:length(P.terms) )

deriv(P::Poly) = Poly([ k*P.terms[k+1] for k ∈ 1:deg(P) ])

function Base.show(io::IO, P::Poly{T} where T)
    first = true
    for i ∈ (1+deg(P)):-1:1
        a = P.terms[i]
        if a != 0
            if !first
                print(" + ")
            end
            if a != 1 || i == 1
                print("(", a, ")")
            end
            if i == 1
            elseif i == 2
                print("X")
            else
                print("X^", i-1)
            end
            first = false
        end
    end
    if first
        print("0")
    end
end

function newton(f, f′, z0::Number, n::Int)
    if n == 0
        z0
    else
        z1 = z0 - f(z0)/f′(z0)
        newton(f, f′, z1, n-1)
    end
end

function main(out)
    n = 3
    roots = [exp(im*2*π*k/n) for k in 1:n]
    P = reduce(*, X - z for z ∈ roots)
    #P = X^8 + 15*X^4 - 16 + 0.0im
    P′ = deriv(P)

    xres, yres = 800, 800
    xrange, yrange = range(-1, 1, xres), range(-1, 1, yres)
    color::Matrix{Int8} = zeros(Int8, xres, yres)
    Threads.@threads for i ∈ 1:xres
        for j ∈ 1:yres
            z0 = xrange[j] + yrange[i]*im
            z = newton(P, P′, z0, 50)
            color[i,j] = findmin((abs(z - z0) for z0 ∈ roots))[2]
        end
    end

    for i ∈ length(yrange):-1:1
        for j ∈ 1:length(xrange)
            print(out, color[i,j], " ")
        end
        print(out, "\n")
    end
end

if abspath(PROGRAM_FILE) == @__FILE__
    # Julia is stupid about buffering?
    out = IOBuffer()
    main(out)
    print(String(take!(out)))
end
