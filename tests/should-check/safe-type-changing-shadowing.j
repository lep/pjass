

globals
    boolean b
endglobals

function A takes nothing returns nothing
    set b = false
endfunction

function C takes nothing returns nothing
    set b = true
endfunction

function B takes nothing returns nothing
    local real b
endfunction
