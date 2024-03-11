type agent extends handle
type boolexpr extends agent
type filterfunc extends boolexpr
native Filter takes code func returns filterfunc

function myF takes nothing returns integer
    return 4
endfunction

function myOtherF takes nothing returns nothing
    call Filter(function myF)
endfunction
