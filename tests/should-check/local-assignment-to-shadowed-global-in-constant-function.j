globals
    integer x
endglobals

constant function foo takes nothing returns nothing
    local integer x
    set x = 5 // Assignment to global variable x in constant function
endfunction