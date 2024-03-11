

function foo takes integer i returns nothing
endfunction

function bar takes nothing returns code
    return function foo(3)
endfunction
