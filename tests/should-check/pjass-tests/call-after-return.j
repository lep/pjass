function x takes nothing returns nothing
endfunction

function foo takes nothing returns integer
    return 1337
    call x()
endfunction
