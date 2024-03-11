function foo1 takes nothing returns integer
    return 0 // <-Compiles correctly
endfunction

function foo2 takes nothing returns integer
    return 0
    // <-This does not
endfunction