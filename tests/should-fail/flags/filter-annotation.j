type agent extends handle
type boolexpr extends agent
type conditionfunc extends boolexpr
type filterfunc extends conditionfunc

native Filter takes code c returns filterfunc

function returnsnoboolean takes nothing returns nothing
endfunction

//# +filter
function foo takes nothing returns nothing
    call Filter(function returnsnoboolean)
endfunction

